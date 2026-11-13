#include "cube.h"
#include "car_mesh.h"
#include "human_mesh.h"
#include "../shader/camera_shader.h"
#include <application/tg_application.h>
#include <string>
#include <vector>
#include <math.h>

TgApplication *getApplication();

#ifndef IMAGES_PATH
#define IMAGES_PATH         "images/"
#endif

// ball: triangles around the ball and from the bottom to the top, the edge of the ball is at most
// 1 - cos(pi/BALL_SEGMENTS) = 0.12% of its radius inside the real ellipsoid
#define BALL_SEGMENTS       64
#define BALL_RINGS          32
// the texture is on the ball this many times around and from the bottom to the top, so a texel is about as large as
// on a cube of the same size, the texture coordinates stay in 0..1 like on the cube faces
#define BALL_TEXTURES_AROUND    4
#define BALL_TEXTURES_UP        2
// cylinder: side quads around it (the same edge accuracy as the ball), the texture is BALL_TEXTURES_AROUND times
// around the side and once from the bottom to the top, the ends have it once like a cube face
#define CYLINDER_SEGMENTS       64
// cone: triangles from the base to the tip around it, the texture like on the cylinder
#define CONE_SEGMENTS           64

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
    m_detailTexture(0),
    m_paintVerticesCount(0),
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
    if (m_detailTexture) {
        glDeleteTextures(1, &m_detailTexture);
    }
}

/*!
 * \brief Cube::setup
 * \param config center position, size, yaw/pitch/roll and texture of the cube, or of the ball (ellipsoid inside
 * the cube), the cylinder or the cone (inside the cube, their axis is the y axis of the cube, the tip of the cone is
 * at the top)
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

    // rotates roll first, then pitch and yaw (the normals of the ball, the cylinder and the cone)
    auto rotate = [=](const float c[3], float out[3]) {
        const float rollX = c[0]*cosf(roll) + c[1]*sinf(roll);
        const float rollY = -c[0]*sinf(roll) + c[1]*cosf(roll);
        const float pitchY = rollY*cosf(pitch) + c[2]*sinf(pitch);
        const float pitchZ = -rollY*sinf(pitch) + c[2]*cosf(pitch);
        out[0] = rollX*cosf(yaw) + pitchZ*sinf(yaw);
        out[1] = pitchY;
        out[2] = -rollX*sinf(yaw) + pitchZ*cosf(yaw);
    };
    // rotates roll first, then pitch and yaw, and moves to the position (the same additions as before the balls, so
    // the cubes do not change)
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
    // ball: the ellipsoid of the semi-axes x1/y1/z1, a point of the sphere angles (ring from the bottom, segment around)
    // with its normal (gradient of the ellipsoid) and the texture coordinate inside its texture copy
    auto addBallPoint = [&](int ring, int segment, int ringBase, int segmentBase) {
        const float up = static_cast<float>(ring)*PI_VALUE/BALL_RINGS;
        const float around = static_cast<float>(segment)*2.0f*PI_VALUE/BALL_SEGMENTS;
        const float local[3] = { x1*sinf(up)*sinf(around), -y1*cosf(up), z1*sinf(up)*cosf(around) };
        const float gradient[3] = { local[0]/(x1*x1), local[1]/(y1*y1), local[2]/(z1*z1) };
        float c[3], n[3];
        transform(local, c);
        rotate(gradient, n);
        const float len = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        const float s = static_cast<float>(segment - segmentBase)*BALL_TEXTURES_AROUND/BALL_SEGMENTS;
        const float t = static_cast<float>(ring - ringBase)*BALL_TEXTURES_UP/BALL_RINGS;
        listVertices.push_back( { c[0], c[1], c[2], n[0]/len, n[1]/len, n[2]/len, s, t, 1.0f, 1.0f, 1.0f, 1.0f } );
    };
    // cylinder: a point of its side at the angle of the segment and the height y with the normal of the side
    // (gradient of the ellipse) or with the normal of its end (endNormal -1 bottom, 1 top)
    auto addCylinderPoint = [&](int segment, float y, float s, float t, float endNormal) {
        const float around = static_cast<float>(segment)*2.0f*PI_VALUE/CYLINDER_SEGMENTS;
        const float local[3] = { x1*sinf(around), y, z1*cosf(around) };
        const float side[3] = { sinf(around)/x1, 0.0f, cosf(around)/z1 };
        const float end[3] = { 0.0f, endNormal, 0.0f };
        float c[3], n[3];
        transform(local, c);
        rotate(endNormal != 0.0f ? end : side, n);
        const float len = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        listVertices.push_back( { c[0], c[1], c[2], n[0]/len, n[1]/len, n[2]/len, s, t, 1.0f, 1.0f, 1.0f, 1.0f } );
    };
    // center of an end of the cylinder
    auto addCylinderCenter = [&](float y, float endNormal) {
        const float local[3] = { 0.0f, y, 0.0f };
        const float end[3] = { 0.0f, endNormal, 0.0f };
        float c[3], n[3];
        transform(local, c);
        rotate(end, n);
        listVertices.push_back( { c[0], c[1], c[2], n[0], n[1], n[2], 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f } );
    };
    // cone: a point of the base (y0) or the tip (y1) at the angle around (radians) with the normal of the side, the
    // side is (x/x1)^2 + (z/z1)^2 = ((y1 - y)/height)^2 and its gradient is along (sin/x1, 1/height, cos/z1), or with
    // the normal of the base (base true)
    auto addConePoint = [&](float around, bool tip, float s, float t, bool base) {
        const float height = y1 - y0;
        const float local[3] = { tip ? 0.0f : x1*sinf(around), tip ? y1 : y0, tip ? 0.0f : z1*cosf(around) };
        const float side[3] = { sinf(around)/x1, 1.0f/height, cosf(around)/z1 };
        const float bottom[3] = { 0.0f, -1.0f, 0.0f };
        float c[3], n[3];
        transform(local, c);
        rotate(base ? bottom : side, n);
        const float len = sqrtf(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
        listVertices.push_back( { c[0], c[1], c[2], n[0]/len, n[1]/len, n[2]/len, s, t, 1.0f, 1.0f, 1.0f, 1.0f } );
    };
    // cars and people are generated meshes, the other shapes are made here
    const bool isMesh = m_config.shape == ObjectShape::Car || m_config.shape == ObjectShape::Suv
            || m_config.shape == ObjectShape::SportCar || m_config.shape == ObjectShape::Man
            || m_config.shape == ObjectShape::Woman || m_config.shape == ObjectShape::Child;
    if (isMesh) {
        // the paint first (texture of the object), then the details (white texture, their vertex colors)
        std::vector<MeshVertex> paint, detail;
        if (m_config.shape == ObjectShape::Man || m_config.shape == ObjectShape::Woman
                || m_config.shape == ObjectShape::Child) {
            const HumanType type = m_config.shape == ObjectShape::Woman ? HumanType::Woman
                    : (m_config.shape == ObjectShape::Child ? HumanType::Child : HumanType::Man);
            buildHumanMesh(type, m_config.size.x, m_config.size.y, m_config.size.z, paint, detail);
        } else {
            const CarType type = m_config.shape == ObjectShape::Suv ? CarType::Suv
                    : (m_config.shape == ObjectShape::SportCar ? CarType::Sport : CarType::Sedan);
            buildCarMesh(type, m_config.size.x, m_config.size.y, m_config.size.z, paint, detail);
        }
        for (const std::vector<MeshVertex> *list : { &paint, &detail }) {
            for (const MeshVertex &v : *list) {
                float c[3], n[3];
                transform(v.position, c);
                rotate(v.normal, n);
                listVertices.push_back( { c[0], c[1], c[2], n[0], n[1], n[2], v.st[0], v.st[1],
                                          v.color[0], v.color[1], v.color[2], v.color[3] } );
            }
        }
        m_paintVerticesCount = static_cast<int>(paint.size());
    } else if (m_config.shape == ObjectShape::Cone) {
        const int segmentsPerTexture = CONE_SEGMENTS/BALL_TEXTURES_AROUND;
        for (int segment=0;segment<CONE_SEGMENTS;segment++) {
            const int segmentBase = segment/segmentsPerTexture*segmentsPerTexture;
            const float s0 = static_cast<float>(segment - segmentBase)*BALL_TEXTURES_AROUND/CONE_SEGMENTS;
            const float s1 = static_cast<float>(segment + 1 - segmentBase)*BALL_TEXTURES_AROUND/CONE_SEGMENTS;
            const float a0 = static_cast<float>(segment)*2.0f*PI_VALUE/CONE_SEGMENTS;
            const float a1 = static_cast<float>(segment + 1)*2.0f*PI_VALUE/CONE_SEGMENTS;
            // the tip has the normal of the middle of the triangle, so the side is smooth around the cone
            addConePoint(a0, false, s0, 0.0f, false);
            addConePoint(a1, false, s1, 0.0f, false);
            addConePoint((a0 + a1)/2.0f, true, (s0 + s1)/2.0f, 1.0f, false);
            // the base has the texture once, the ellipse inside the texture square
            addCylinderCenter(y0, -1.0f);
            addConePoint(a0, false, 0.5f + 0.5f*sinf(a0), 0.5f + 0.5f*cosf(a0), true);
            addConePoint(a1, false, 0.5f + 0.5f*sinf(a1), 0.5f + 0.5f*cosf(a1), true);
        }
    } else if (m_config.shape == ObjectShape::Cylinder) {
        const int segmentsPerTexture = CYLINDER_SEGMENTS/BALL_TEXTURES_AROUND;
        for (int segment=0;segment<CYLINDER_SEGMENTS;segment++) {
            const int segmentBase = segment/segmentsPerTexture*segmentsPerTexture;
            const float s0 = static_cast<float>(segment - segmentBase)*BALL_TEXTURES_AROUND/CYLINDER_SEGMENTS;
            const float s1 = static_cast<float>(segment + 1 - segmentBase)*BALL_TEXTURES_AROUND/CYLINDER_SEGMENTS;
            addCylinderPoint(segment, y0, s0, 0.0f, 0.0f);
            addCylinderPoint(segment + 1, y0, s1, 0.0f, 0.0f);
            addCylinderPoint(segment + 1, y1, s1, 1.0f, 0.0f);
            addCylinderPoint(segment, y0, s0, 0.0f, 0.0f);
            addCylinderPoint(segment + 1, y1, s1, 1.0f, 0.0f);
            addCylinderPoint(segment, y1, s0, 1.0f, 0.0f);
            // the ends have the texture once, the ellipse inside the texture square
            for (const float endNormal : { -1.0f, 1.0f }) {
                const float y = endNormal < 0.0f ? y0 : y1;
                const float a0 = static_cast<float>(segment)*2.0f*PI_VALUE/CYLINDER_SEGMENTS;
                const float a1 = static_cast<float>(segment + 1)*2.0f*PI_VALUE/CYLINDER_SEGMENTS;
                addCylinderCenter(y, endNormal);
                addCylinderPoint(segment, y, 0.5f + 0.5f*sinf(a0), 0.5f + 0.5f*cosf(a0), endNormal);
                addCylinderPoint(segment + 1, y, 0.5f + 0.5f*sinf(a1), 0.5f + 0.5f*cosf(a1), endNormal);
            }
        }
    } else if (m_config.shape == ObjectShape::Ball) {
        const int segmentsPerTexture = BALL_SEGMENTS/BALL_TEXTURES_AROUND;
        const int ringsPerTexture = BALL_RINGS/BALL_TEXTURES_UP;
        for (int ring=0;ring<BALL_RINGS;ring++) {
            const int ringBase = ring/ringsPerTexture*ringsPerTexture;
            for (int segment=0;segment<BALL_SEGMENTS;segment++) {
                const int segmentBase = segment/segmentsPerTexture*segmentsPerTexture;
                // the quad has a corner at the bottom and the top point, so it is one triangle there
                if (ring > 0) {
                    addBallPoint(ring, segment, ringBase, segmentBase);
                    addBallPoint(ring, segment + 1, ringBase, segmentBase);
                    addBallPoint(ring + 1, segment + 1, ringBase, segmentBase);
                }
                if (ring + 1 < BALL_RINGS) {
                    addBallPoint(ring, segment, ringBase, segmentBase);
                    addBallPoint(ring + 1, segment + 1, ringBase, segmentBase);
                    addBallPoint(ring + 1, segment, ringBase, segmentBase);
                }
            }
        }
    } else {
        addFace(front);
        addFace(back);
        addFace(left);
        addFace(right);
        addFace(top);
        addFace(bottom);
    }

    m_verticesCount = static_cast<int>(listVertices.size());
    if (!isMesh) {
        m_paintVerticesCount = m_verticesCount;
    }
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
    if (m_paintVerticesCount < m_verticesCount) {
        const unsigned char white[4] = { 255, 255, 255, 255 };
        glGenTextures(1, &m_detailTexture);
        glBindTexture(GL_TEXTURE_2D, m_detailTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
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
    glDrawArrays(GL_TRIANGLES, 0, m_paintVerticesCount);
    if (m_detailTexture) {
        glBindTexture(GL_TEXTURE_2D, m_detailTexture);
        glDrawArrays(GL_TRIANGLES, m_paintVerticesCount, m_verticesCount - m_paintVerticesCount);
    }
    glBindVertexArray(0);
}
