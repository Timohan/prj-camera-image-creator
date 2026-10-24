#ifndef SKY_H
#define SKY_H

#include <GL/glew.h>
#include <GL/gl.h>

/*!
 * \brief Sky
 * Bowl of clear sky above the floor, from SKY_LOWEST_ELEVATION degrees below
 * the horizon to the zenith. It is drawn without lights and without the earth curve.
 */
class Sky
{
public:
    explicit Sky();
    ~Sky();

    int init();
    void render();

private:
    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;
    GLuint m_textureIndex;
    int m_verticesCount;
};

#endif // SKY_H
