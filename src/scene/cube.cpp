#include "cube.h"
#include "../shader/camera_shader.h"
#include <application/tg_application.h>
#include <vector>
#include <math.h>

TgApplication *getApplication();

#ifndef IMAGES_PATH
#define IMAGES_PATH         "images/"
#endif

struct CubeVertice
{
    float x, y, z;
    float nx, ny, nz;
    float s, t;
    float r, g, b, a;
};

/*!
 * \brief Cube::Cube
 * \param x, z center of the cube on the floor
 * \param y bottom of the cube
 */
Cube::Cube(float x, float y, float z, float width, float height, float deep) :
    m_x(x),
    m_y(y),
    m_z(z),
    m_width(width),
    m_height(height),
    m_deep(deep),
    m_vertexArrayObject(0),
    m_vertexBufferObject(0),
    m_textureIndex(0),
    m_verticesCount(0)
{
}

Cube::~Cube()
{
    if (m_vertexArrayObject) {
        glDeleteVertexArrays(1, &m_vertexArrayObject);
    }
    if (m_vertexBufferObject) {
        glDeleteBuffers(1, &m_vertexBufferObject);
    }
}

int Cube::init()
{
    const float x0 = m_x - m_width/2.0f;
    const float x1 = m_x + m_width/2.0f;
    const float y0 = m_y;
    const float y1 = m_y + m_height;
    const float z0 = m_z - m_deep/2.0f;
    const float z1 = m_z + m_deep/2.0f;
    std::vector<CubeVertice> listVertices;

    // every face has 4 corners given counter-clockwise when looking at the face from outside
    auto addFace = [&listVertices](const float c[4][3]) {
        const float st[4][2] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
        const int order[6] = { 0, 1, 2, 0, 2, 3 };
        // outward normal of the face
        const float e1[3] = { c[1][0]-c[0][0], c[1][1]-c[0][1], c[1][2]-c[0][2] };
        const float e2[3] = { c[2][0]-c[1][0], c[2][1]-c[1][1], c[2][2]-c[1][2] };
        float n[3] = { e1[1]*e2[2]-e1[2]*e2[1], e1[2]*e2[0]-e1[0]*e2[2], e1[0]*e2[1]-e1[1]*e2[0] };
        const float len = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        n[0] /= len;
        n[1] /= len;
        n[2] /= len;
        for (int i : order) {
            listVertices.push_back( { c[i][0], c[i][1], c[i][2], n[0], n[1], n[2], st[i][0], st[i][1], 1.0f, 1.0f, 1.0f, 1.0f } );
        }
    };
    const float front[4][3]  = { {x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0} };
    const float back[4][3]   = { {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1} };
    const float left[4][3]   = { {x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0} };
    const float right[4][3]  = { {x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1} };
    const float top[4][3]    = { {x0, y1, z0}, {x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0} };
    const float bottom[4][3] = { {x0, y0, z1}, {x0, y0, z0}, {x1, y0, z0}, {x1, y0, z1} };
    addFace(front);
    addFace(back);
    addFace(left);
    addFace(right);
    addFace(top);
    addFace(bottom);

    m_verticesCount = static_cast<int>(listVertices.size());
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertice)*m_verticesCount, listVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord], 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor], 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(8 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glBindVertexArray(0);

    m_textureIndex = getApplication()->getTextureIndex(IMAGES_PATH "wood.png");
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

/*!
 * \brief Cube::getCenter
 * \return middle point of the cube
 */
VECTOR3 Cube::getCenter() const
{
    VECTOR3 center;
    center.x = m_x;
    center.y = m_y + m_height/2.0f;
    center.z = m_z;
    return center;
}

void Cube::render()
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
