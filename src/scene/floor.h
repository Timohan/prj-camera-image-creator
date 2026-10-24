#ifndef FLOOR_H
#define FLOOR_H

#include <GL/glew.h>
#include <GL/gl.h>

// floor reaches 5 km to every direction, about the distance to the horizon for human eyes on a flat ground
#define FLOOR_HALF_SIZE     5000.0f
// floor and direction lines are made of pieces of this size, so they can be bent by the earth curve
#define FLOOR_SEGMENT_SIZE  50.0f

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
