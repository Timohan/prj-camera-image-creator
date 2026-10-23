#include "floor.h"
#include "../shader/camera_shader.h"
#include <application/tg_application.h>
#include <vector>

TgApplication *getApplication();

#ifndef IMAGES_PATH
#define IMAGES_PATH         "images/"
#endif

#define FLOOR_HALF_SIZE     10.0f
// floor is divided into FLOOR_TILE_COUNT x FLOOR_TILE_COUNT tiles, each tile has the whole texture
#define FLOOR_TILE_COUNT    5

struct FloorVertice
{
    float x, y, z;
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
    // rectangle -10/0/10, 10/0/10, 10/0/-10, -10/0/-10 divided into tiles,
    // every other tile column/row has reversed texture, so tile edges match each other
    std::vector<FloorVertice> listVertices;
    const float tileSize = FLOOR_HALF_SIZE*2.0f/static_cast<float>(FLOOR_TILE_COUNT);
    int x, z;

    for (z=0;z<FLOOR_TILE_COUNT;z++) {
        const float z0 = -FLOOR_HALF_SIZE + static_cast<float>(z)*tileSize;
        const float z1 = z0 + tileSize;
        const float t0 = (z % 2) ? 1.0f : 0.0f;
        const float t1 = 1.0f - t0;
        for (x=0;x<FLOOR_TILE_COUNT;x++) {
            const float x0 = -FLOOR_HALF_SIZE + static_cast<float>(x)*tileSize;
            const float x1 = x0 + tileSize;
            const float s0 = (x % 2) ? 1.0f : 0.0f;
            const float s1 = 1.0f - s0;

            listVertices.push_back( { x0, 0.0f, z1,   s0, t1,   1.0f, 1.0f, 1.0f, 1.0f } );
            listVertices.push_back( { x1, 0.0f, z1,   s1, t1,   1.0f, 1.0f, 1.0f, 1.0f } );
            listVertices.push_back( { x1, 0.0f, z0,   s1, t0,   1.0f, 1.0f, 1.0f, 1.0f } );

            listVertices.push_back( { x0, 0.0f, z1,   s0, t1,   1.0f, 1.0f, 1.0f, 1.0f } );
            listVertices.push_back( { x1, 0.0f, z0,   s1, t0,   1.0f, 1.0f, 1.0f, 1.0f } );
            listVertices.push_back( { x0, 0.0f, z0,   s0, t0,   1.0f, 1.0f, 1.0f, 1.0f } );
        }
    }

    m_verticesCount = static_cast<int>(listVertices.size());
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(FloorVertice)*m_verticesCount, listVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition], 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord], 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor], 4, GL_FLOAT, GL_FALSE, 9 * sizeof(float), reinterpret_cast<void*>(5 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glBindVertexArray(0);

    m_textureIndex = getApplication()->getTextureIndex(IMAGES_PATH "floor.png");
    if (!m_textureIndex) {
        return 1;
    }
    // texture is loaded with nearest filtering, use smooth filtering with mipmaps for the floor
    glBindTexture(GL_TEXTURE_2D, m_textureIndex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
}

void Floor::render()
{
    glBindVertexArray(m_vertexArrayObject);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord]);
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glBindTexture(GL_TEXTURE_2D, m_textureIndex);
    glDrawArrays(GL_TRIANGLES, 0, m_verticesCount);
    glBindVertexArray(0);
}
