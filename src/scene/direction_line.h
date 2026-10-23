#ifndef DIRECTION_LINE_H
#define DIRECTION_LINE_H

#include <GL/glew.h>
#include <GL/gl.h>

/*!
 * \brief DirectionLine
 * Two lines on the floor that cross at the center of the floor: one from z -10 to 10
 * and one from x -10 to 10. They show where is front, rear, left and right.
 */
class DirectionLine
{
public:
    explicit DirectionLine();
    ~DirectionLine();

    void setup(float width, float height);

    int init();
    void render();

private:
    float m_width;
    float m_height;
    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;
    GLuint m_textureIndex;
    int m_verticesCount;
};

#endif // DIRECTION_LINE_H
