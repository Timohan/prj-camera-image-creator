#include "floor.h"
#include "../shader/camera_shader.h"
#include <application/tg_application.h>
#include <vector>

TgApplication *getApplication();

#ifndef IMAGES_PATH
#define IMAGES_PATH         "images/"
#endif

// each FLOOR_TILE_SIZE x FLOOR_TILE_SIZE tile has the whole texture
#define FLOOR_TILE_SIZE     4.0f
// tiles start from this position, so the tiles are at same place as on the 20x20 floor
#define FLOOR_TILE_START    -10.0f

struct FloorVertice
{
    float x, y, z;
    float nx, ny, nz;
    float s, t;
    float r, g, b, a;
};

Floor::Floor() :
    m_vertexArrayObject(0),
    m_vertexBufferObject(0),
    m_textureIndex(0),
    m_verticesCount(0)
{
}

Floor::~Floor()
{
    if (m_vertexArrayObject) {
        glDeleteVertexArrays(1, &m_vertexArrayObject);
    }
    if (m_vertexBufferObject) {
        glDeleteBuffers(1, &m_vertexBufferObject);
    }
}

int Floor::init()
{
    // grid of FLOOR_SEGMENT_SIZE squares, so the shader can bend it by the earth curve,
    // texture is repeated on every tile and every other tile column/row
    // has reversed texture (mirrored repeat), so tile edges match each other
    std::vector<FloorVertice> listVertices;
    const int segmentCount = static_cast<int>(FLOOR_HALF_SIZE*2.0f/FLOOR_SEGMENT_SIZE);

    for (int z=0;z<segmentCount;z++) {
        const float z0 = -FLOOR_HALF_SIZE + static_cast<float>(z)*FLOOR_SEGMENT_SIZE;
        const float z1 = z0 + FLOOR_SEGMENT_SIZE;
        const float t0 = (z0 - FLOOR_TILE_START)/FLOOR_TILE_SIZE;
        const float t1 = (z1 - FLOOR_TILE_START)/FLOOR_TILE_SIZE;
        for (int x=0;x<segmentCount;x++) {
            const float x0 = -FLOOR_HALF_SIZE + static_cast<float>(x)*FLOOR_SEGMENT_SIZE;
            const float x1 = x0 + FLOOR_SEGMENT_SIZE;
            const float s0 = (x0 - FLOOR_TILE_START)/FLOOR_TILE_SIZE;
            const float s1 = (x1 - FLOOR_TILE_START)/FLOOR_TILE_SIZE;

            listVertices.push_back( { x0, 0.0f, z1,   0.0f, 1.0f, 0.0f,  s0, t1,   1.0f, 1.0f, 1.0f, 1.0f } );
            listVertices.push_back( { x1, 0.0f, z1,   0.0f, 1.0f, 0.0f,  s1, t1,   1.0f, 1.0f, 1.0f, 1.0f } );
            listVertices.push_back( { x1, 0.0f, z0,   0.0f, 1.0f, 0.0f,  s1, t0,   1.0f, 1.0f, 1.0f, 1.0f } );

            listVertices.push_back( { x0, 0.0f, z1,   0.0f, 1.0f, 0.0f,  s0, t1,   1.0f, 1.0f, 1.0f, 1.0f } );
            listVertices.push_back( { x1, 0.0f, z0,   0.0f, 1.0f, 0.0f,  s1, t0,   1.0f, 1.0f, 1.0f, 1.0f } );
            listVertices.push_back( { x0, 0.0f, z0,   0.0f, 1.0f, 0.0f,  s0, t0,   1.0f, 1.0f, 1.0f, 1.0f } );
        }
    }

    m_verticesCount = static_cast<int>(listVertices.size());
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(FloorVertice)*m_verticesCount, listVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord], 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor], 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(8 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glBindVertexArray(0);

    m_textureIndex = getApplication()->getTextureIndex(IMAGES_PATH "floor.png");
    if (!m_textureIndex) {
        return 1;
    }
    // texture is loaded with nearest filtering and clamping, use smooth filtering with mipmaps for the floor
    glBindTexture(GL_TEXTURE_2D, m_textureIndex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    if (GLEW_EXT_texture_filter_anisotropic) {
        // keeps the floor sharper when it is seen at low angle far away
        GLfloat maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
}

void Floor::render()
{
    glBindVertexArray(m_vertexArrayObject);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal]);
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord]);
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glBindTexture(GL_TEXTURE_2D, m_textureIndex);
    glDrawArrays(GL_TRIANGLES, 0, m_verticesCount);
    glBindVertexArray(0);
}
