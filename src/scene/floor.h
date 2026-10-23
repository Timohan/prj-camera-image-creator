#ifndef FLOOR_H
#define FLOOR_H

#include <GL/glew.h>
#include <GL/gl.h>

class Floor
{
public:
    explicit Floor();
    ~Floor();

    int init();
    void render();

private:
    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;
    GLuint m_textureIndex;
    int m_verticesCount;
};

#endif // FLOOR_H
