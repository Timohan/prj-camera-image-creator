#include "visibility_map.h"
#include <GL/gl.h>
#include <algorithm>
#include "global_log.h"
#include <math.h>
#include <string>
#include "camera_image_saver.h"
#include "cameraobject.h"
#include "../shader/camera_shader.h"

// size of one cube face of the distance map in pixels
#define VISIBILITY_FACE_SIZE        1024
// cube face sees this many pixels more on every side than 90 degrees, so the face edges have pixels
#define VISIBILITY_FACE_MARGIN      2.0f
#define VISIBILITY_FACE_COUNT       6
#define VISIBILITY_NEAR_PLANE       0.01f
#define VISIBILITY_FAR_PLANE        10000.0f
// distance map value of the pixels without a surface (sky)
#define VISIBILITY_NOTHING          -1.0f
// texture units of the distance maps, 0 is for the textures of the objects and 1 for the shadow map
#define VISIBILITY_FIRST_TEXTURE_UNIT   2

VisibilityMap::VisibilityMap() :
    m_faceSize(VISIBILITY_FACE_SIZE)
{
}

VisibilityMap::~VisibilityMap()
{
    clear();
}

void VisibilityMap::clear()
{
    for (CameraVisibility &camera : m_listCamera) {
        glDeleteTextures(1, &camera.texture);
    }
    m_listCamera.clear();
}

/*!
 * \brief VisibilityMap::create
 * renders the distance maps of the cameras, they must be created again if the scene changes
 * \param listCamera camera objects in the resolution of the saved images,
 * at most CAMERA_SHADER_MAX_VISIBILITY_CAMERAS are used
 * \param render renders the distance from the camera
 * \return 0 on success
 */
int VisibilityMap::create(const std::vector<CameraObjectConfig> &listCamera, const RenderFunction &render)
{
    clear();
    unbind();
    const int count = std::min(static_cast<int>(listCamera.size()), CAMERA_SHADER_MAX_VISIBILITY_CAMERAS);
    if (count < static_cast<int>(listCamera.size())) {
        TG_ERROR_LOG("Only", CAMERA_SHADER_MAX_VISIBILITY_CAMERAS, "cameras are used for the visibility");
    }
    GLint maxTextureSize, maxRenderbufferSize, maxViewport[2];
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewport);
    m_faceSize = std::min({ VISIBILITY_FACE_SIZE, static_cast<int>(maxTextureSize)/VISIBILITY_FACE_COUNT,
                            static_cast<int>(maxRenderbufferSize)/VISIBILITY_FACE_COUNT, static_cast<int>(maxViewport[1]) });
    const int width = m_faceSize*VISIBILITY_FACE_COUNT;
    const float tanHalf = 1.0f + VISIBILITY_FACE_MARGIN*2.0f/static_cast<float>(m_faceSize);

    GLint previousFramebuffer, previousViewport[4], oldClampFragmentColor;
    GLfloat previousClearColor[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, previousClearColor);
    glGetIntegerv(GL_CLAMP_FRAGMENT_COLOR, &oldClampFragmentColor);
    const GLboolean oldBlend = glIsEnabled(GL_BLEND);
    // distances are larger than 1.0, so colors must not be clamped or blended
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
    glDisable(GL_BLEND);

    GLuint framebuffer, depthbuffer;
    glGenFramebuffers(1, &framebuffer);
    glGenRenderbuffers(1, &depthbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, m_faceSize);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);

    Matrix4x4 projection;
    Math3D::MatrixPerspectiveFovLH(&projection, 2.0f*atanf(tanHalf), 1.0f, VISIBILITY_NEAR_PLANE, VISIBILITY_FAR_PLANE);
    int result = 0;
    for (int i=0;i<count && result == 0;i++) {
        const CameraObjectConfig &config = listCamera[static_cast<size_t>(i)];
        CameraObject cameraObject;
        cameraObject.setup(config, 1.0f);
        CameraVisibility camera;
        camera.position = cameraObject.getPosition();
        camera.forward = cameraObject.getDirection();
        camera.up = cameraObject.getUpVector();
        // right side of the image like in the fisheye image of CameraImageSaver
        camera.right = Math3D::Vec3Normalize(Math3D::Vec3Cross(camera.up, camera.forward));
        const CameraImageSaver::LensLimits limits = CameraImageSaver::getLensLimits(config);
        camera.lens[0] = config.focalLength*config.resolutionWidth/config.sensorWidth;
        camera.lens[1] = config.focalLength*config.resolutionHeight/config.sensorHeight;
        camera.lens[2] = config.principalPointX;
        camera.lens[3] = config.principalPointY;
        camera.size[0] = roundf(config.resolutionWidth);
        camera.size[1] = roundf(config.resolutionHeight);
        camera.size[2] = config.fisheye ? 1.0f : 0.0f;
        camera.size[3] = config.fisheye ? limits.maxTheta : limits.maxRadius2;
        if (config.fisheye) {
            std::copy(config.fisheyeDistortion, config.fisheyeDistortion + 4, camera.distortion);
            std::fill(camera.distortion + 4, camera.distortion + 8, 0.0f);
        } else {
            // OpenCV order like CameraImageSaver
            const GLfloat distortion[8] = { config.radialDistortion[0], config.radialDistortion[1],
                                            config.tangentialDistortion[0], config.tangentialDistortion[1],
                                            config.radialDistortion[2], config.radialDistortion[3],
                                            config.radialDistortion[4], config.radialDistortion[5] };
            std::copy(distortion, distortion + 8, camera.distortion);
        }
        std::copy(limits.area, limits.area + 4, camera.area);

        glGenTextures(1, &camera.texture);
        glBindTexture(GL_TEXTURE_2D, camera.texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, m_faceSize, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        m_listCamera.push_back(camera);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, camera.texture, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            TG_ERROR_LOG("Cannot create visibility framebuffer", std::to_string(width) + "x" + std::to_string(m_faceSize));
            result = 1;
            break;
        }
        glViewport(0, 0, width, m_faceSize);
        glClearColor(VISIBILITY_NOTHING, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // faces in the order of the shader: forward, right, left, backward, up, down
        auto negative = [](const VECTOR3 &v) { VECTOR3 n = { -v.x, -v.y, -v.z }; return n; };
        const VECTOR3 faceDirections[VISIBILITY_FACE_COUNT] = { camera.forward, camera.right, negative(camera.right),
                                                                negative(camera.forward), camera.up, negative(camera.up) };
        const VECTOR3 faceUps[VISIBILITY_FACE_COUNT] = { camera.up, camera.up, camera.up, camera.up,
                                                         negative(camera.forward), camera.forward };
        for (int face=0;face<VISIBILITY_FACE_COUNT;face++) {
            VECTOR3 at;
            at.x = camera.position.x + faceDirections[face].x;
            at.y = camera.position.y + faceDirections[face].y;
            at.z = camera.position.z + faceDirections[face].z;
            Matrix4x4 view;
            Math3D::MatrixLookAtLH(&view, camera.position, at, faceUps[face]);
            glViewport(face*m_faceSize, 0, m_faceSize, m_faceSize);
            render(projection, view, camera.position, i);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteRenderbuffers(1, &depthbuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    glClearColor(previousClearColor[0], previousClearColor[1], previousClearColor[2], previousClearColor[3]);
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, static_cast<GLenum>(oldClampFragmentColor));
    if (oldBlend) {
        glEnable(GL_BLEND);
    }
    if (result != 0) {
        clear();
    }
    return result;
}

/*!
 * \brief VisibilityMap::setupShader
 * binds the distance maps and sets the cameras to the shader
 * \param enabled false: the shader does not black out anything and the maps are not bound
 */
void VisibilityMap::setupShader(GLuint shader, bool enabled) const
{
    const bool used = enabled && !m_listCamera.empty();
    glUniform1f(glGetUniformLocation(shader, "hideUnseen"), used ? 1.0f : 0.0f);
    if (!used) {
        return;
    }
    const float tanHalf = 1.0f + VISIBILITY_FACE_MARGIN*2.0f/static_cast<float>(m_faceSize);
    glUniform1i(glGetUniformLocation(shader, "visibilityCameraCount"), static_cast<GLint>(m_listCamera.size()));
    glUniform1f(glGetUniformLocation(shader, "visibilityTanHalf"), tanHalf);
    // angle of one texel at the center of the face
    glUniform1f(glGetUniformLocation(shader, "visibilityTexelAngle"), 2.0f*tanHalf/static_cast<float>(m_faceSize));
    for (size_t i=0;i<m_listCamera.size();i++) {
        const CameraVisibility &camera = m_listCamera[i];
        const std::string index = "[" + std::to_string(i) + "]";
        glUniform3f(glGetUniformLocation(shader, ("visibilityPosition" + index).c_str()),
                    camera.position.x, camera.position.y, camera.position.z);
        glUniform3f(glGetUniformLocation(shader, ("visibilityRight" + index).c_str()), camera.right.x, camera.right.y, camera.right.z);
        glUniform3f(glGetUniformLocation(shader, ("visibilityUp" + index).c_str()), camera.up.x, camera.up.y, camera.up.z);
        glUniform3f(glGetUniformLocation(shader, ("visibilityForward" + index).c_str()),
                    camera.forward.x, camera.forward.y, camera.forward.z);
        glUniform4fv(glGetUniformLocation(shader, ("visibilityLens" + index).c_str()), 1, camera.lens);
        glUniform4fv(glGetUniformLocation(shader, ("visibilitySize" + index).c_str()), 1, camera.size);
        glUniform4fv(glGetUniformLocation(shader, ("visibilityDistortionA" + index).c_str()), 1, camera.distortion);
        glUniform4fv(glGetUniformLocation(shader, ("visibilityDistortionB" + index).c_str()), 1, camera.distortion + 4);
        glUniform4fv(glGetUniformLocation(shader, ("visibilityArea" + index).c_str()), 1, camera.area);
        const GLint unit = VISIBILITY_FIRST_TEXTURE_UNIT + static_cast<GLint>(i);
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
        glBindTexture(GL_TEXTURE_2D, camera.texture);
        glUniform1i(glGetUniformLocation(shader, ("visibilityMap" + std::to_string(i)).c_str()), unit);
    }
    glActiveTexture(GL_TEXTURE0);
}

/*!
 * \brief VisibilityMap::unbind
 * unbinds the distance maps, so they are not read while the maps or the saved images are rendered
 */
void VisibilityMap::unbind() const
{
    for (int i=0;i<CAMERA_SHADER_MAX_VISIBILITY_CAMERAS;i++) {
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(VISIBILITY_FIRST_TEXTURE_UNIT + i));
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glActiveTexture(GL_TEXTURE0);
}
