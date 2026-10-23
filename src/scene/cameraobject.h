#ifndef CAMERA_OBJECT_H
#define CAMERA_OBJECT_H

#include <GL/glew.h>
#include <GL/gl.h>
#include "../math/math3d.h"

/*!
 * \brief CameraObject
 * Camera drawn as a right-angled (rectangular) cone. The rectangle area is
 * the lens that is pointed to the target, apex is behind the camera.
 */
class CameraObject
{
public:
    explicit CameraObject(VECTOR3 position, VECTOR3 target, float width, float height, float deep);
    ~CameraObject();

    int init();
    void render();

private:
    VECTOR3 m_position;
    VECTOR3 m_target;
    float m_width, m_height, m_deep;
    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;
    GLuint m_textureIndex;
    int m_verticesCount;
};

#endif // CAMERA_OBJECT_H
