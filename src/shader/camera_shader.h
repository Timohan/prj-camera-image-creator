/*!
 * \file
 * \brief file camera_shader.h
 *
 * Compile shader
 *
 * Copyright of Timo Hannukkala. All rights reserved.
 *
 * \author Timo Hannukkala <timohannukkala@hotmail.com>
 */

#ifndef CAMERA_SHADER_H
#define CAMERA_SHADER_H

#include <GL/glew.h>
#include <GL/gl.h>

// maximum count of spot lights in the shader
#define CAMERA_SHADER_MAX_SPOT_LIGHTS   8

enum ShaderAttributes3d
{
    AttribPosition = 0,
    AttribNormal,
    AttribTextCoord,
    AttribColor
};

class CameraShader
{
public:
    CameraShader();
    ~CameraShader();

    int init();

    GLuint generalShader();

    static GLuint m_shaderAttributeIndex[4];
private:
    static const char *m_vertShader;
    static const char *m_fragShader;
    GLuint m_generalShader;

    static int compileShader(const char *source, const GLenum type, GLuint *shader);
};

#endif // CAMERA_SHADER_H
