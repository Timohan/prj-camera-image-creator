/*!
 * \file
 * \brief file camera_shader.cpp
 *
 * Compile shader
 *
 * Copyright of Timo Hannukkala. All rights reserved.
 *
 * \author Timo Hannukkala <timohannukkala@hotmail.com>
 */

#include "camera_shader.h"
#include <iostream>

GLuint CameraShader::m_shaderAttributeIndex[4] = {0,0,0,0};

/*!
 * \brief CameraShader::m_vertShader
 *
 * simple 3D vertex shader
 */
const char *CameraShader::m_vertShader = "attribute vec3 vertex;" \
        "attribute vec3 normal;" \
        "attribute vec2 tex_coord;" \
        "attribute vec4 color;" \
        "varying vec3 worldPosition;" \
        "varying vec3 worldNormal;" \
        "varying vec2 texCoord;" \
        "varying vec4 colorLight;" \
        "uniform mat4 view;" \
        "uniform mat4 projection;" \
        "uniform vec3 transformPosition;" \
        "void main()" \
        "{" \
        "    worldNormal             = normal;" \
        "    texCoord                = tex_coord;" \
        "    colorLight              = color;" \
        "    vec3 pos                = transformPosition+vertex;" \
        "    worldPosition           = pos;" \
        "    gl_Position             = projection*(view*(vec4(pos, 1.0)));" \
        "}";
/*!
 * \brief CameraShader::m_fragShader
 *
 * fragment shader
 */
const char *CameraShader::m_fragShader = "" \
        "uniform sampler2D texture;" \
        "uniform vec3 lightPosition;" \
        "uniform vec3 lightDirection;" \
        "uniform vec3 lightColor;" \
        "uniform float lightCosInner;" \
        "uniform float lightCosOuter;" \
        "uniform float ambientFactor;" \
        "varying vec3 worldPosition;" \
        "varying vec3 worldNormal;" \
        "varying vec2 texCoord;" \
        "varying vec4 colorLight;" \
        "void main()" \
        "{" \
        "       vec3 toLight  = normalize(lightPosition-worldPosition);" \
        "       float diffuse = max(dot(normalize(worldNormal), toLight), 0.0);" \
        "       float cosAngle = dot(-toLight, normalize(lightDirection));" \
        "       float spot    = smoothstep(lightCosOuter, lightCosInner, cosAngle);" \
        "       vec3 light    = lightColor*(ambientFactor + diffuse*spot);" \
        "       vec4 color    = texture2D(texture, texCoord)*colorLight;" \
        "       gl_FragColor  = vec4(color.rgb*light, color.a);" \
        "}";
/*!
 * \brief CameraShader::CameraShader
 *
 * constructor
 */
CameraShader::CameraShader() :
    m_generalShader(0)
{
}

/*!
 * \brief CameraShader::~CameraShader
 *
 * destructor
 */
CameraShader::~CameraShader()
{
    if (m_generalShader) {
        glDeleteProgram(m_generalShader);
    }
}

/*!
 * \brief CameraShader::generalShader
 * \return general shader
 */
GLuint CameraShader::generalShader()
{
    return m_generalShader;
}

/*!
 * \brief CameraShader::init
 *
 * loads the shaders
 *
 * \return 0 on success
 */
int CameraShader::init()
{
    GLuint shaderHandle = glCreateProgram();
    GLint status;
    GLuint fragShader;
    GLuint vertShader;

    if (compileShader(m_vertShader, GL_VERTEX_SHADER, &vertShader) != 0) {
        std::cerr << "Compile shader failed vert" << std::endl;
        return -1;
    }
    glAttachShader(shaderHandle, vertShader);
    glDeleteShader(vertShader);

    if (compileShader(m_fragShader, GL_FRAGMENT_SHADER, &fragShader) != 0) {
        std::cerr << "Compile shader failed frag" << std::endl;
        return -1;
    }
    glAttachShader(shaderHandle, fragShader);
    glDeleteShader(fragShader);

    glLinkProgram(shaderHandle);

    glGetProgramiv(shaderHandle, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        std::cerr << "The link operation is failed" << std::endl;
        return -1;
    }
    m_generalShader = shaderHandle;
    m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition] = glGetAttribLocation(shaderHandle, "vertex");
    m_shaderAttributeIndex[ShaderAttributes3d::AttribNormal] = glGetAttribLocation(shaderHandle, "normal");
    m_shaderAttributeIndex[ShaderAttributes3d::AttribTextCoord] = glGetAttribLocation(shaderHandle, "tex_coord");
    m_shaderAttributeIndex[ShaderAttributes3d::AttribColor] = glGetAttribLocation(shaderHandle, "color");
    return 0;
}

/*!
 * \brief CameraShader::compileShader
 *
 * compile the shader
 *
 * \param source, source text of the shader
 * \param type, type of the shader (vertex or frag)
 * \param shader, returns shader here, if return is 0
 * \return 0 on success
 */
int CameraShader::compileShader(const char *source, const GLenum type, GLuint *shader)
{
    GLint status;
    GLuint shaderHandle = glCreateShader(type);

    if (shaderHandle == 0) {
        std::cerr << "Creating the shader failed" << std::endl;
        return -1;
    }

    glShaderSource(shaderHandle, 1, &source, nullptr);
    glCompileShader(shaderHandle);

    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &status);

    if(status == GL_FALSE) {
        char infoLog[512];
        glGetShaderInfoLog(shaderHandle, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Compiling shader failed: " << infoLog << std::endl;
        glDeleteShader(shaderHandle);
        return -1;
    }

    *shader = shaderHandle;
    return 0;
}
