#ifndef CUBE_H
#define CUBE_H

#include <GL/glew.h>
#include <GL/gl.h>

class Cube
{
public:
    explicit Cube(float x, float y, float z, float width, float height, float deep);
    ~Cube();

    int init();
    void render();

private:
    float m_x, m_y, m_z;
    float m_width, m_height, m_deep;
    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;
    GLuint m_textureIndex;
    int m_verticesCount;
};

#endif // CUBE_H
