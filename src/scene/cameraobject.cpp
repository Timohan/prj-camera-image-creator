#include "cameraobject.h"
#include "../shader/camera_shader.h"
#include <application/tg_application.h>
#include <vector>
#include <math.h>

TgApplication *getApplication();

#ifndef IMAGES_PATH
#define IMAGES_PATH         "images/"
#endif

// size of the cone when sizeRate is 1
#define CAMERA_OBJECT_WIDTH     0.6f
#define CAMERA_OBJECT_HEIGHT    0.6f
#define CAMERA_OBJECT_DEEP      0.6f

struct CameraObjectVertice
{
    float x, y, z;
    float nx, ny, nz;
    float s, t;
    float r, g, b, a;
};

CameraObject::CameraObject() :
    m_sizeRate(1.0f),
    m_vertexArrayObject(0),
    m_vertexBufferObject(0),
    m_textureIndex(0),
    m_verticesCount(0)
{
}

/*!
 * \brief CameraObject::setup
 * \param config position is the middle of the rectangle area (lens) of the cone,
 *               the rectangle area is pointed to the direction and rolled by roll
 * \param sizeRate size of the cone compared to the default size (0.5 = half size)
 */
void CameraObject::setup(const CameraObjectConfig &config, float sizeRate)
{
    m_config = config;
    m_sizeRate = sizeRate;
}

VECTOR3 CameraObject::getPosition() const
{
    return m_config.position;
}

/*!
 * \brief CameraObject::getDirection
 * \return normalized direction
 */
VECTOR3 CameraObject::getDirection() const
{
    return Math3D::Vec3Normalize(m_config.direction);
}

/*!
 * \brief CameraObject::getUpVector
 * \return up vector of the camera with the roll
 */
VECTOR3 CameraObject::getUpVector() const
{
    VECTOR3 right, top;
    getAxes(&right, &top);
    return top;
}

/*!
 * \brief CameraObject::getAspectRatio
 * \return width/height of the camera image
 */
float CameraObject::getAspectRatio() const
{
    return m_config.resolutionWidth/m_config.resolutionHeight;
}

/*!
 * \brief CameraObject::getProjectionMatrix
 * left-handed projection of a pinhole camera, same depth range as Math3D::MatrixPerspectiveFovLH.
 * Focal length in pixels is fx = focalLength*resolutionWidth/sensorWidth and
 * fy = focalLength*resolutionHeight/sensorHeight, principal point moves the image center.
 * \param zNear near plane
 * \param zFar far plane
 * \return projection matrix
 */
Matrix4x4 CameraObject::getProjectionMatrix(float zNear, float zFar) const
{
    Matrix4x4 projection;
    const float width = m_config.resolutionWidth;
    const float height = m_config.resolutionHeight;
    const float fx = m_config.focalLength*width/m_config.sensorWidth;
    const float fy = m_config.focalLength*height/m_config.sensorHeight;

    for (int i=0;i<16;i++) {
        projection.m[i] = 0.0f;
    }
    Math3D::setMatrixValue(&projection, 0, 0, 2.0f*fx/width);
    Math3D::setMatrixValue(&projection, 1, 1, 2.0f*fy/height);
    // principal point is measured from the top left corner of the image, y axis down
    Math3D::setMatrixValue(&projection, 2, 0, 2.0f*m_config.principalPointX/width - 1.0f);
    Math3D::setMatrixValue(&projection, 2, 1, 1.0f - 2.0f*m_config.principalPointY/height);
    Math3D::setMatrixValue(&projection, 2, 2, zFar/(zFar-zNear));
    Math3D::setMatrixValue(&projection, 2, 3, 1.0f);
    Math3D::setMatrixValue(&projection, 3, 2, -zNear*zFar/(zFar-zNear));
    return projection;
}

/*!
 * \brief CameraObject::getAxes
 * calculates right and top axis of the camera, top is rotated by the roll
 * \param right returns right axis
 * \param top returns top axis
 */
void CameraObject::getAxes(VECTOR3 *right, VECTOR3 *top) const
{
    VECTOR3 up = { 0.0f, 1.0f, 0.0f };
    const VECTOR3 forward = getDirection();
    if (fabsf(forward.y) > 0.999f) {
        // looking straight up or down, cannot use y as up vector
        up = { 0.0f, 0.0f, 1.0f };
    }
    const VECTOR3 unrolledRight = Math3D::Vec3Normalize(Math3D::Vec3Cross(up, forward));
    const VECTOR3 unrolledTop = Math3D::Vec3Cross(forward, unrolledRight);
    const float roll = m_config.roll*PI_VALUE/180.0f;
    const float c = cosf(roll);
    const float s = sinf(roll);
    // positive roll tilts the top to the right (clockwise seen from behind the camera)
    right->x = unrolledRight.x*c - unrolledTop.x*s;
    right->y = unrolledRight.y*c - unrolledTop.y*s;
    right->z = unrolledRight.z*c - unrolledTop.z*s;
    top->x = unrolledTop.x*c + unrolledRight.x*s;
    top->y = unrolledTop.y*c + unrolledRight.y*s;
    top->z = unrolledTop.z*c + unrolledRight.z*s;
}

CameraObject::~CameraObject()
{
    if (m_vertexArrayObject) {
        glDeleteVertexArrays(1, &m_vertexArrayObject);
    }
    if (m_vertexBufferObject) {
        glDeleteBuffers(1, &m_vertexBufferObject);
    }
}

int CameraObject::init()
{
    const VECTOR3 forward = getDirection();
    VECTOR3 right, top;
    getAxes(&right, &top);
    const float halfW = CAMERA_OBJECT_WIDTH*m_sizeRate/2.0f;
    const float halfH = CAMERA_OBJECT_HEIGHT*m_sizeRate/2.0f;
    const float deep = CAMERA_OBJECT_DEEP*m_sizeRate;

    const VECTOR3 &position = m_config.position;
    auto point = [&position, &right, &top](float w, float h) {
        VECTOR3 p;
        p.x = position.x + right.x*w + top.x*h;
        p.y = position.y + right.y*w + top.y*h;
        p.z = position.z + right.z*w + top.z*h;
        return p;
    };
    // base corners (counter-clockwise looking from the target) and apex behind the base
    const VECTOR3 c[4] = { point(halfW, -halfH), point(-halfW, -halfH), point(-halfW, halfH), point(halfW, halfH) };
    VECTOR3 apex;
    apex.x = position.x - forward.x*deep;
    apex.y = position.y - forward.y*deep;
    apex.z = position.z - forward.z*deep;

    std::vector<CameraObjectVertice> listVertices;
    // adds a triangle with outward normal, p0, p1, p2 are counter-clockwise looking from outside
    auto addTriangle = [&listVertices](const VECTOR3 p[3], const float st[3][2]) {
        VECTOR3 e1 = Math3D::decreaceVector(p[1], p[0]);
        VECTOR3 e2 = Math3D::decreaceVector(p[2], p[1]);
        VECTOR3 n = Math3D::Vec3Normalize(Math3D::Vec3Cross(e1, e2));
        for (int i=0;i<3;i++) {
            listVertices.push_back( { p[i].x, p[i].y, p[i].z, n.x, n.y, n.z, st[i][0], st[i][1], 1.0f, 1.0f, 1.0f, 1.0f } );
        }
    };

    // rectangle area facing the target
    const float baseSt[4][2] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
    const VECTOR3 base1[3] = { c[0], c[1], c[2] };
    const float baseSt1[3][2] = { {baseSt[0][0], baseSt[0][1]}, {baseSt[1][0], baseSt[1][1]}, {baseSt[2][0], baseSt[2][1]} };
    addTriangle(base1, baseSt1);
    const VECTOR3 base2[3] = { c[0], c[2], c[3] };
    const float baseSt2[3][2] = { {baseSt[0][0], baseSt[0][1]}, {baseSt[2][0], baseSt[2][1]}, {baseSt[3][0], baseSt[3][1]} };
    addTriangle(base2, baseSt2);
    // four sides from the base edges to the apex
    const float sideSt[3][2] = { {0, 0}, {1, 0}, {0.5f, 1} };
    for (int i=0;i<4;i++) {
        const VECTOR3 side[3] = { c[(i+1)%4], c[i], apex };
        addTriangle(side, sideSt);
    }

    m_verticesCount = static_cast<int>(listVertices.size());
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(CameraObjectVertice)*m_verticesCount, listVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal], 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord], 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor], 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float), reinterpret_cast<void*>(8 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glBindVertexArray(0);

    m_textureIndex = getApplication()->getTextureIndex(IMAGES_PATH "cameraobject.png");
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

void CameraObject::render()
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
