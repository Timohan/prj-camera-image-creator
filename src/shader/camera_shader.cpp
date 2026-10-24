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

#define SHADER_VALUE_TEXT(value)    #value
#define SHADER_VALUE(value)         SHADER_VALUE_TEXT(value)

/*!
 * \brief CameraShader::m_vertShader
 *
 * 3D vertex shader. When earthCurve is 1.0, the flat world is bent on a sphere of
 * earthRadius: point at horizontal distance r from the origin and height y moves to
 * angle theta = r/earthRadius on the sphere, its height is along the sphere normal and
 * its normal is rotated by theta. Small angle series are used, because
 * earthRadius*(1 - cos(theta)) loses the precision of float.
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
        "uniform float earthRadius;" \
        "uniform float earthCurve;" \
        "void main()" \
        "{" \
        "    texCoord                = tex_coord;" \
        "    colorLight              = color;" \
        "    vec3 pos                = transformPosition+vertex;" \
        "    vec3 nor                = normal;" \
        "    float r                 = length(pos.xz);" \
        "    if (earthCurve > 0.5 && r > 0.0) {" \
        "        vec2 dir            = pos.xz/r;" \
        "        float theta         = r/earthRadius;" \
        "        float theta2        = theta*theta;" \
        "        float sinTheta      = theta*(1.0 - theta2/6.0);" \
        "        float cosTheta      = 1.0 - theta2/2.0;" \
        "        float drop          = r*r/(2.0*earthRadius)*(1.0 - theta2/12.0);" \
        "        float horizontal    = r*(1.0 - theta2/6.0) + pos.y*sinTheta;" \
        "        pos                 = vec3(dir.x*horizontal, pos.y*cosTheta - drop, dir.y*horizontal);" \
        "        float radial        = dot(nor.xz, dir);" \
        "        vec2 tangent        = nor.xz - dir*radial;" \
        "        float newRadial     = radial*cosTheta + nor.y*sinTheta;" \
        "        nor                 = vec3(tangent.x + dir.x*newRadial, nor.y*cosTheta - radial*sinTheta, tangent.y + dir.y*newRadial);" \
        "    }" \
        "    worldNormal             = nor;" \
        "    worldPosition           = pos;" \
        "    gl_Position             = projection*(view*(vec4(pos, 1.0)));" \
        "}";
/*!
 * \brief CameraShader::m_fragShader
 *
 * fragment shader, ambient light and up to CAMERA_SHADER_MAX_SPOT_LIGHTS spot lights
 */
const char *CameraShader::m_fragShader = "" \
        "#define MAX_SPOT_LIGHTS " SHADER_VALUE(CAMERA_SHADER_MAX_SPOT_LIGHTS) "\n" \
        "uniform sampler2D texture;" \
        "uniform int lightCount;" \
        "uniform vec3 lightPosition[MAX_SPOT_LIGHTS];" \
        "uniform vec3 lightDirection[MAX_SPOT_LIGHTS];" \
        "uniform vec3 lightColor[MAX_SPOT_LIGHTS];" \
        "uniform float lightCosInner[MAX_SPOT_LIGHTS];" \
        "uniform float lightCosOuter[MAX_SPOT_LIGHTS];" \
        "uniform vec3 ambientLight;" \
        "varying vec3 worldPosition;" \
        "varying vec3 worldNormal;" \
        "varying vec2 texCoord;" \
        "varying vec4 colorLight;" \
        "void main()" \
        "{" \
        "       vec3 normal   = normalize(worldNormal);" \
        "       vec3 light    = ambientLight;" \
        "       for (int i=0;i<MAX_SPOT_LIGHTS;i++) {" \
        "           if (i < lightCount) {" \
        "               vec3 toLight   = normalize(lightPosition[i]-worldPosition);" \
        "               float diffuse  = max(dot(normal, toLight), 0.0);" \
        "               float cosAngle = dot(-toLight, normalize(lightDirection[i]));" \
        "               float spot     = smoothstep(lightCosOuter[i], lightCosInner[i], cosAngle);" \
        "               light          += lightColor[i]*diffuse*spot;" \
        "           }" \
        "       }" \
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
