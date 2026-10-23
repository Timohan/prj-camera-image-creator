#ifndef CUBE_H
#define CUBE_H

#include <GL/glew.h>
#include <GL/gl.h>
#include "objects_config.h"

class Cube
{
public:
    explicit Cube();
    ~Cube();

    void setup(const CubeConfig &config);
    int init();
    void render();

private:
    CubeConfig m_config;
    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;
    GLuint m_textureIndex;
    int m_verticesCount;
};

#endif // CUBE_H
