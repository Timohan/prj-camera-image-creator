#include "sky.h"
#include "../shader/camera_shader.h"
#include "../math/math3d.h"
#include <application/tg_application.h>
#include <vector>
#include <math.h>

TgApplication *getApplication();

#ifndef IMAGES_PATH
#define IMAGES_PATH             "images/"
#endif

// sky is inside the far plane and outside of the floor corners
#define SKY_RADIUS              9000.0f
// sky continues below the horizon, so the horizon of the curved floor has no gap
#define SKY_LOWEST_ELEVATION    -10.0f
#define SKY_AZIMUTH_SEGMENTS    64
#define SKY_ELEVATION_SEGMENTS  32

struct SkyVertice
{
    float x, y, z;
    float nx, ny, nz;
    float s, t;
    float r, g, b, a;
};

Sky::Sky() :
    m_vertexArrayObject(0),
    m_vertexBufferObject(0),
    m_textureIndex(0),
    m_verticesCount(0)
{
}

Sky::~Sky()
{
    if (m_vertexArrayObject) {
        glDeleteVertexArrays(1, &m_vertexArrayObject);
    }
    if (m_vertexBufferObject) {
        glDeleteBuffers(1, &m_vertexBufferObject);
    }
}

int Sky::init()
{
    // sky.png top row is the zenith and bottom row is SKY_LOWEST_ELEVATION
    std::vector<SkyVertice> listVertices;
    auto vertice = [](int azimuthIndex, int elevationIndex) {
        const float rate = static_cast<float>(elevationIndex)/static_cast<float>(SKY_ELEVATION_SEGMENTS);
        const float elevation = (SKY_LOWEST_ELEVATION + (90.0f - SKY_LOWEST_ELEVATION)*rate)*PI_VALUE/180.0f;
        const float azimuth = static_cast<float>(azimuthIndex)/static_cast<float>(SKY_AZIMUTH_SEGMENTS)*2.0f*PI_VALUE;
        const float x = cosf(elevation)*sinf(azimuth);
        const float y = sinf(elevation);
        const float z = cosf(elevation)*cosf(azimuth);
        // normal points to the center, where the sky is seen from
        return SkyVertice { x*SKY_RADIUS, y*SKY_RADIUS, z*SKY_RADIUS, -x, -y, -z,
                            static_cast<float>(azimuthIndex)/static_cast<float>(SKY_AZIMUTH_SEGMENTS), 1.0f - rate,
                            1.0f, 1.0f, 1.0f, 1.0f };
    };
    for (int e=0;e<SKY_ELEVATION_SEGMENTS;e++) {
        for (int a=0;a<SKY_AZIMUTH_SEGMENTS;a++) {
            listVertices.push_back(vertice(a, e));
            listVertices.push_back(vertice(a + 1, e));
            listVertices.push_back(vertice(a + 1, e + 1));

            listVertices.push_back(vertice(a, e));
            listVertices.push_back(vertice(a + 1, e + 1));
            listVertices.push_back(vertice(a, e + 1));
        }
    }

    m_verticesCount = static_cast<int>(listVertices.size());
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(SkyVertice)*m_verticesCount, listVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord], 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor], 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(8 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glBindVertexArray(0);

    m_textureIndex = getApplication()->getTextureIndex(IMAGES_PATH "sky.png");
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

void Sky::render()
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
