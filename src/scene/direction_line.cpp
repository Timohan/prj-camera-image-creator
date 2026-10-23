#include "direction_line.h"
#include "../shader/camera_shader.h"
#include <application/tg_application.h>
#include <vector>

TgApplication *getApplication();

#ifndef IMAGES_PATH
#define IMAGES_PATH         "images/"
#endif

// lines are as long as the floor
#define DIRECTION_LINE_HALF_LENGTH  10.0f

struct DirectionLineVertice
{
    float x, y, z;
    float nx, ny, nz;
    float s, t;
    float r, g, b, a;
};

DirectionLine::DirectionLine() :
    m_width(0.1f),
    m_height(0.001f),
    m_vertexArrayObject(0),
    m_vertexBufferObject(0),
    m_textureIndex(0),
    m_verticesCount(0)
{
}

DirectionLine::~DirectionLine()
{
    if (m_vertexArrayObject) {
        glDeleteVertexArrays(1, &m_vertexArrayObject);
    }
    if (m_vertexBufferObject) {
        glDeleteBuffers(1, &m_vertexBufferObject);
    }
}

/*!
 * \brief DirectionLine::setup
 * \param width width of the lines
 * \param height height position of the lines from the floor
 */
void DirectionLine::setup(float width, float height)
{
    m_width = width;
    m_height = height;
}

int DirectionLine::init()
{
    const float half = m_width/2.0f;
    const float length = DIRECTION_LINE_HALF_LENGTH;
    std::vector<DirectionLineVertice> listVertices;

    // rectangle that faces up
    auto addRectangle = [this, &listVertices](float x0, float z0, float x1, float z1) {
        const float c[4][5] = {
            { x0, z1, 0, 0 },
            { x1, z1, 1, 0 },
            { x1, z0, 1, 1 },
            { x0, z0, 0, 1 },
        };
        const int order[6] = { 0, 1, 2, 0, 2, 3 };
        for (int i : order) {
            listVertices.push_back( { c[i][0], m_height, c[i][1], 0.0f, 1.0f, 0.0f, c[i][2], c[i][3], 1.0f, 1.0f, 1.0f, 1.0f } );
        }
    };
    // front-rear line
    addRectangle(-half, -length, half, length);
    // left-right line, the crossing square is already drawn by the line above
    addRectangle(-length, -half, -half, half);
    addRectangle(half, -half, length, half);

    m_verticesCount = static_cast<int>(listVertices.size());
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(DirectionLineVertice)*m_verticesCount, listVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord], 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor], 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(8 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glBindVertexArray(0);

    m_textureIndex = getApplication()->getTextureIndex(IMAGES_PATH "direction_line.png");
    if (!m_textureIndex) {
        return 1;
    }
    // texture is loaded with nearest filtering, use smooth filtering with mipmaps
    glBindTexture(GL_TEXTURE_2D, m_textureIndex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return 0;
}

void DirectionLine::render()
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
