#ifndef CAMERA_OBJECT_H
#define CAMERA_OBJECT_H

#include <GL/glew.h>
#include <GL/gl.h>
#include "../math/math3d.h"
#include "cameraobjects_config.h"

/*!
 * \brief CameraObject
 * Camera drawn as a right-angled (rectangular) cone. The rectangle area is
 * the lens that is pointed to the direction, apex is behind the camera.
 */
class CameraObject
{
public:
    explicit CameraObject();
    ~CameraObject();

    void setup(const CameraObjectConfig &config, float sizeRate);
    VECTOR3 getPosition() const;
    VECTOR3 getDirection() const;
    VECTOR3 getUpVector() const;
    float getAspectRatio() const;
    Matrix4x4 getProjectionMatrix(float zNear, float zFar) const;
    Matrix4x4 getImageProjectionMatrix(float left, float top, float width, float height, float zNear, float zFar) const;
    Matrix4x4 getViewMatrix() const;
    const CameraObjectConfig &getConfig() const;

    int init();
    void render();

private:
    CameraObjectConfig m_config;
    float m_sizeRate;
    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;
    GLuint m_textureIndex;
    int m_verticesCount;

    void getAxes(VECTOR3 *right, VECTOR3 *top) const;
};

#endif // CAMERA_OBJECT_H
