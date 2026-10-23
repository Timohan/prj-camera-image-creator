#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <GL/glew.h>
#include <GL/gl.h>

class Triangle
{
public:
    explicit Triangle();
    ~Triangle();

    int init();
    void render();

private:
    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;
    int m_verticesCount;
};

#endif // TRIANGLE_H
