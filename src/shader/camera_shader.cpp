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
 * fragment shader, ambient light and up to CAMERA_SHADER_MAX_SPOT_LIGHTS spot lights with shadows,
 * when sky is 1.0 spot lights are not used and the light is ambientLight*skyAmbientRate,
 * when distancePass is 1.0 red is the distance from distanceFrom to the pixel (float framebuffer),
 * when distancePass is 2.0 (surface pass) rgb is the normal of the surface turned toward distanceFrom
 * and alpha is surfaceId, the number of the object (float framebuffer).
 * Shadow of the spot light: the pixel is moved toward its normal by SHADOW_NORMAL_OFFSET texels of the
 * shadow map (no shadow acne), it is projected to the tile of the light in the shadow map and
 * compared with 3x3 linear filtered samples (soft edge).
 * When hideUnseen is 1.0, pixels that no camera object sees are black (VisibilityMap): a camera sees the pixel
 * if the lens of the camera puts it inside the saved image and the distance map of the camera has no nearer
 * surface in its direction, the pixel is moved toward its normal by VISIBILITY_NORMAL_OFFSET texels like
 * in the shadow (far away the moved pixel can be above the horizon, where nothing hides it).
 * Sky is seen if the distance map has no surface in its direction.
 */
const char *CameraShader::m_fragShader = "" \
        "#define MAX_SPOT_LIGHTS " SHADER_VALUE(CAMERA_SHADER_MAX_SPOT_LIGHTS) "\n" \
        "#define SHADOW_NORMAL_OFFSET 1.5\n" \
        "#define MAX_VISIBILITY_CAMERAS " SHADER_VALUE(CAMERA_SHADER_MAX_VISIBILITY_CAMERAS) "\n" \
        "#define VISIBILITY_NORMAL_OFFSET 2.0\n" \
        "#define VISIBILITY_DISTANCE_TOLERANCE 2.0\n" \
        "uniform sampler2D texture;" \
        "uniform sampler2DShadow shadowMap;" \
        "uniform vec2 shadowMapTexelSize;" \
        "uniform int lightCount;" \
        "uniform vec3 lightPosition[MAX_SPOT_LIGHTS];" \
        "uniform vec3 lightDirection[MAX_SPOT_LIGHTS];" \
        "uniform vec3 lightColor[MAX_SPOT_LIGHTS];" \
        "uniform float lightCosInner[MAX_SPOT_LIGHTS];" \
        "uniform float lightCosOuter[MAX_SPOT_LIGHTS];" \
        "uniform mat4 lightProjection[MAX_SPOT_LIGHTS];" \
        "uniform mat4 lightView[MAX_SPOT_LIGHTS];" \
        "uniform vec4 lightShadowTile[MAX_SPOT_LIGHTS];" \
        "uniform float lightShadowTexelRate[MAX_SPOT_LIGHTS];" \
        "uniform vec3 ambientLight;" \
        "uniform float sky;" \
        "uniform float skyAmbientRate;" \
        "uniform float distancePass;" \
        "uniform vec3 distanceFrom;" \
        "uniform float surfaceId;" \
        "uniform float hideUnseen;" \
        "uniform int visibilityCameraCount;" \
        "uniform float visibilityTanHalf;" \
        "uniform float visibilityTexelAngle;" \
        "uniform vec3 visibilityPosition[MAX_VISIBILITY_CAMERAS];" \
        "uniform vec3 visibilityRight[MAX_VISIBILITY_CAMERAS];" \
        "uniform vec3 visibilityUp[MAX_VISIBILITY_CAMERAS];" \
        "uniform vec3 visibilityForward[MAX_VISIBILITY_CAMERAS];" \
        "uniform vec4 visibilityLens[MAX_VISIBILITY_CAMERAS];" \
        "uniform vec4 visibilitySize[MAX_VISIBILITY_CAMERAS];" \
        "uniform vec4 visibilityDistortionA[MAX_VISIBILITY_CAMERAS];" \
        "uniform vec4 visibilityDistortionB[MAX_VISIBILITY_CAMERAS];" \
        "uniform vec4 visibilityArea[MAX_VISIBILITY_CAMERAS];" \
        "uniform sampler2D visibilityMap0;" \
        "uniform sampler2D visibilityMap1;" \
        "uniform sampler2D visibilityMap2;" \
        "uniform sampler2D visibilityMap3;" \
        "uniform sampler2D visibilityMap4;" \
        "uniform sampler2D visibilityMap5;" \
        "uniform sampler2D visibilityMap6;" \
        "uniform sampler2D visibilityMap7;" \
        "varying vec3 worldPosition;" \
        "varying vec3 worldNormal;" \
        "varying vec2 texCoord;" \
        "varying vec4 colorLight;" \
        "float getShadow(int i, vec3 normal)" \
        "{" \
        "       float toLight   = length(lightPosition[i]-worldPosition);" \
        "       vec3 position   = worldPosition + normal*toLight*lightShadowTexelRate[i]*SHADOW_NORMAL_OFFSET;" \
        "       vec4 projected  = lightProjection[i]*(lightView[i]*vec4(position, 1.0));" \
        "       if (projected.w <= 0.0) {" \
        "           return 1.0;" \
        "       }" \
        "       vec3 ndc        = projected.xyz/projected.w;" \
        "       if (abs(ndc.x) > 1.0 || abs(ndc.y) > 1.0) {" \
        "           return 1.0;" \
        "       }" \
        "       vec4 tile       = lightShadowTile[i];" \
        "       vec2 uv         = tile.xy + (ndc.xy*0.5 + 0.5)*tile.zw;" \
        "       float depth     = min(ndc.z*0.5 + 0.5, 1.0);" \
        "       vec2 minUv      = tile.xy + shadowMapTexelSize;" \
        "       vec2 maxUv      = tile.xy + tile.zw - shadowMapTexelSize;" \
        "       float lit       = 0.0;" \
        "       for (int y=-1;y<=1;y++) {" \
        "           for (int x=-1;x<=1;x++) {" \
        "               vec2 sampleUv = clamp(uv + vec2(float(x), float(y))*shadowMapTexelSize, minUv, maxUv);" \
        "               lit           += shadow2D(shadowMap, vec3(sampleUv, depth)).r;" \
        "           }" \
        "       }" \
        "       return lit/9.0;" \
        "}" \
        "bool insideImage(int i, vec3 ray)" \
        "{" \
        "       float x       = dot(ray, visibilityRight[i]);" \
        "       float y       = -dot(ray, visibilityUp[i]);" \
        "       float z       = dot(ray, visibilityForward[i]);" \
        "       vec4 lens     = visibilityLens[i];" \
        "       vec4 size     = visibilitySize[i];" \
        "       vec4 a        = visibilityDistortionA[i];" \
        "       vec4 b        = visibilityDistortionB[i];" \
        "       vec2 pixel;" \
        "       if (size.z > 0.5) {" \
        "           float r       = length(vec2(x, y));" \
        "           float theta   = atan(r, z);" \
        "           if (theta > size.w) {" \
        "               return false;" \
        "           }" \
        "           float t2      = theta*theta;" \
        "           float thetaD  = theta*(1.0 + t2*(a.x + t2*(a.y + t2*(a.z + t2*a.w))));" \
        "           pixel         = r > 0.0 ? vec2(x, y)/r*thetaD : vec2(0.0);" \
        "       } else {" \
        "           if (z <= 0.0) {" \
        "               return false;" \
        "           }" \
        "           vec2 n        = vec2(x, y)/z;" \
        "           float r2      = dot(n, n);" \
        "           if (r2 > size.w) {" \
        "               return false;" \
        "           }" \
        "           vec2 undistorted = n*lens.xy + lens.zw;" \
        "           vec4 area     = visibilityArea[i];" \
        "           if (undistorted.x < area.x || undistorted.y < area.y || undistorted.x >= area.z || undistorted.y >= area.w) {" \
        "               return false;" \
        "           }" \
        "           float radial  = (1.0 + r2*(a.x + r2*(a.y + r2*b.x)))/(1.0 + r2*(b.y + r2*(b.z + r2*b.w)));" \
        "           pixel         = vec2(n.x*radial + 2.0*a.z*n.x*n.y + a.w*(r2 + 2.0*n.x*n.x)," \
        "                                n.y*radial + a.z*(r2 + 2.0*n.y*n.y) + 2.0*a.w*n.x*n.y);" \
        "       }" \
        "       pixel         = pixel*lens.xy + lens.zw;" \
        "       return pixel.x >= -0.5 && pixel.y >= -0.5 && pixel.x < size.x - 0.5 && pixel.y < size.y - 0.5;" \
        "}" \
        "float getVisibilityDistance(int i, sampler2D map, vec3 ray)" \
        "{" \
        "       float x       = dot(ray, visibilityRight[i]);" \
        "       float u       = dot(ray, visibilityUp[i]);" \
        "       float z       = dot(ray, visibilityForward[i]);" \
        "       vec3 axis     = abs(vec3(x, u, z));" \
        "       float face;" \
        "       vec2 xy;" \
        "       if (axis.z >= axis.x && axis.z >= axis.y) {" \
        "           face          = z > 0.0 ? 0.0 : 3.0;" \
        "           xy            = vec2(x/z, u/abs(z));" \
        "       } else if (axis.x >= axis.y) {" \
        "           face          = x > 0.0 ? 1.0 : 2.0;" \
        "           xy            = vec2(-z/x, u/abs(x));" \
        "       } else {" \
        "           face          = u > 0.0 ? 4.0 : 5.0;" \
        "           xy            = vec2(x/abs(u), -z/u);" \
        "       }" \
        "       vec2 st       = (xy/visibilityTanHalf + 1.0)*0.5;" \
        "       return texture2D(map, vec2((face + st.x)/6.0, st.y)).r;" \
        "}" \
        "bool seenBy(int i, sampler2D map, vec3 normal)" \
        "{" \
        "       vec3 toPixel  = worldPosition - visibilityPosition[i];" \
        "       if (!insideImage(i, toPixel)) {" \
        "           return false;" \
        "       }" \
        "       if (sky > 0.5) {" \
        "           return getVisibilityDistance(i, map, toPixel) < 0.0;" \
        "       }" \
        "       float pixelDistance = length(toPixel);" \
        "       vec3 toward   = dot(normal, toPixel) > 0.0 ? -normal : normal;" \
        "       vec3 moved    = toPixel + toward*pixelDistance*visibilityTexelAngle*VISIBILITY_NORMAL_OFFSET;" \
        "       float nearest = getVisibilityDistance(i, map, moved);" \
        "       return nearest < 0.0 || length(moved) <= nearest + pixelDistance*visibilityTexelAngle*VISIBILITY_DISTANCE_TOLERANCE;" \
        "}" \
        "bool seenByAnyCamera(vec3 normal)" \
        "{" \
        "       return (visibilityCameraCount > 0 && seenBy(0, visibilityMap0, normal))" \
        "           || (visibilityCameraCount > 1 && seenBy(1, visibilityMap1, normal))" \
        "           || (visibilityCameraCount > 2 && seenBy(2, visibilityMap2, normal))" \
        "           || (visibilityCameraCount > 3 && seenBy(3, visibilityMap3, normal))" \
        "           || (visibilityCameraCount > 4 && seenBy(4, visibilityMap4, normal))" \
        "           || (visibilityCameraCount > 5 && seenBy(5, visibilityMap5, normal))" \
        "           || (visibilityCameraCount > 6 && seenBy(6, visibilityMap6, normal))" \
        "           || (visibilityCameraCount > 7 && seenBy(7, visibilityMap7, normal));" \
        "}" \
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
        "               if (diffuse*spot > 0.0) {" \
        "                   light      += lightColor[i]*diffuse*spot*getShadow(i, normal);" \
        "               }" \
        "           }" \
        "       }" \
        "       if (sky > 0.5) {" \
        "           light     = ambientLight*skyAmbientRate;" \
        "       }" \
        "       vec4 color    = texture2D(texture, texCoord)*colorLight;" \
        "       gl_FragColor  = vec4(color.rgb*light, color.a);" \
        "       if (distancePass > 1.5) {" \
        "           gl_FragColor = vec4(dot(normal, worldPosition - distanceFrom) > 0.0 ? -normal : normal, surfaceId);" \
        "       } else if (distancePass > 0.5) {" \
        "           gl_FragColor = vec4(length(worldPosition - distanceFrom), 0.0, 0.0, 1.0);" \
        "       }" \
        "       if (hideUnseen > 0.5 && distancePass < 0.5 && !seenByAnyCamera(normal)) {" \
        "           gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);" \
        "       }" \
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
