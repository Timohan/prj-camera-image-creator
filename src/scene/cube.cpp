#include "cube.h"
#include "../shader/camera_shader.h"
#include <application/tg_application.h>
#include <string>
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

Cube::Cube() :
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

/*!
 * \brief Cube::setup
 * \param config center position, size, yaw/pitch/roll and texture of the cube
 */
void Cube::setup(const CubeConfig &config)
{
    m_config = config;
}

int Cube::init()
{
    // corners are first relative to the center, they are rotated and moved to the position
    const float x0 = -m_config.size.x/2.0f;
    const float x1 = m_config.size.x/2.0f;
    const float y0 = -m_config.size.y/2.0f;
    const float y1 = m_config.size.y/2.0f;
    const float z0 = -m_config.size.z/2.0f;
    const float z1 = m_config.size.z/2.0f;
    const float yaw = m_config.yaw*PI_VALUE/180.0f;
    const float pitch = m_config.pitch*PI_VALUE/180.0f;
    const float roll = m_config.roll*PI_VALUE/180.0f;
    const VECTOR3 &position = m_config.position;
    std::vector<CubeVertice> listVertices;

    // rotates roll first, then pitch and yaw, and moves to the position
    auto transform = [=](const float c[3], float out[3]) {
        const float rollX = c[0]*cosf(roll) + c[1]*sinf(roll);
        const float rollY = -c[0]*sinf(roll) + c[1]*cosf(roll);
        const float pitchY = rollY*cosf(pitch) + c[2]*sinf(pitch);
        const float pitchZ = -rollY*sinf(pitch) + c[2]*cosf(pitch);
        out[0] = position.x + rollX*cosf(yaw) + pitchZ*sinf(yaw);
        out[1] = position.y + pitchY;
        out[2] = position.z - rollX*sinf(yaw) + pitchZ*cosf(yaw);
    };

    // every face has 4 corners given counter-clockwise when looking at the face from outside
    auto addFace = [&listVertices, &transform](const float corners[4][3]) {
        float c[4][3];
        for (int i=0;i<4;i++) {
            transform(corners[i], c[i]);
        }
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

    m_textureIndex = getApplication()->getTextureIndex((std::string(IMAGES_PATH) + m_config.texture).c_str());
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
