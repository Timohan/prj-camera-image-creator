#include "base_scene.h"
#include "../math/math3d.h"
#include "cameraobjects_config.h"
#include "general_config.h"
#include "camera_image_saver.h"
#include "global_log.h"
#include <filesystem>
#include <time.h>
#include <math.h>
#include <algorithm>

// vertical field of view, wide enough to see the floor below the camera
#ifndef CONFIGS_PATH
#define CONFIGS_PATH            "configs/"
#endif

// saved images are rendered at most this many times larger in width and height and shrunk
#define MAX_SUPERSAMPLING       4

#define CAMERA_FIELD_OF_VIEW    (PI_VALUE*2.0f/3.0f)
#define MM_TO_OPENGL_UNIT       1000.0f
#define CAMERA_NEAR_PLANE       0.1f
// floor, direction lines and cubes are bent by the curve of the earth
#define EARTH_RADIUS            6371000.0f
// sky brightness is the ambient light multiplied by this
#define SKY_AMBIENT_RATE        2.0f
// far plane reaches the corners of the floor
#define CAMERA_FAR_PLANE        10000.0f
// surface numbers of the surface pass: 0 is nothing (sky), direction lines are painted on the floor,
// so they are the same surface, then the cubes and then the camera objects
#define SURFACE_FLOOR           1
#define SURFACE_FIRST_CUBE      2
// picked distances: pixel without objects (sky) and pixel outside of the image of the camera object view
#define PICK_SKY                -1.0f
#define PICK_OUTSIDE            -2.0f
// hiddenCameraObject of renderScene() that hides all camera objects: saved images and the visibility maps never have
// the camera objects (the training images of prj-camera-image-learner do not have them)
#define NO_CAMERA_OBJECTS       -2

BaseScene::BaseScene(const std::string &objectsFile, int camerasVisible) :
    m_directionLineVisible(true),
    m_listCube(nullptr),
    m_cubeCount(0),
    m_ambientLight({ 0.0f, 0.0f, 0.0f }),
    m_hideUnseen(false),
    m_visibilityMapChanged(true),
    m_listCameraObject(nullptr),
    m_cameraCount(0),
    m_cameraObjectsVisible(true),
    m_cameraIndex(0),
    m_objectsFile(objectsFile),
    m_imageOutputScale(1.0f),
    m_saveImagesRequested(false),
    m_saveDataRequested(false),
    m_moveSpeed(1000.0f),
    m_moveSpeedMouseWheel(1000.0f),
    m_yawDirection(0),
    m_pitchDirection(0),
    m_pickPending(false),
    m_pickX(0.0f),
    m_pickY(0.0f),
    m_picked(false),
    m_pickedWindowX(0),
    m_pickedWindowY(0),
    m_pickedCamera(0),
    m_pickedDistance(PICK_OUTSIDE)
{
    setupVisualPosition();
    // camerasVisible of the command line is used instead of general.ini
    if (camerasVisible >= 0) {
        m_cameraObjectsVisible = camerasVisible != 0;
    }
    createCameraObjects();
    createObjects();
}

BaseScene::~BaseScene()
{
    delete[] m_listCameraObject;
    delete[] m_listCube;
}

int BaseScene::initCustomShader()
{
    if (m_shader.init()) {
        return 1;
    }
    glUseProgram(m_shader.generalShader() );
    if (m_sky.init()
        || m_floor.init()
        || m_directionLine.init()) {
        return 1;
    }
    for (int i=0;i<m_cubeCount;i++) {
        if (m_listCube[i].init()) {
            return 1;
        }
    }
    for (int i=0;i<m_cameraCount;i++) {
        if (m_listCameraObject[i].init()) {
            return 1;
        }
    }
    return createShadowMap();
}

/*!
 * \brief BaseScene::createShadowMap
 * renders the shadow maps of the spot lights, cubes cast the shadows, cubes and spot lights do not move
 * \return 0 on success
 */
int BaseScene::createShadowMap()
{
    const GLuint shader = m_shader.generalShader();
    auto render = [this, shader](const Matrix4x4 &projection, const Matrix4x4 &view) {
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, projection.m);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, view.m);
        glUniform3f(glGetUniformLocation(shader, "transformPosition"), 0, 0, 0);
        glUniform1f(glGetUniformLocation(shader, "earthRadius"), EARTH_RADIUS);
        glUniform1f(glGetUniformLocation(shader, "earthCurve"), 1.0f);
        // only the depth is used, no lights, so the shadow map is not read
        glUniform1i(glGetUniformLocation(shader, "lightCount"), 0);
        glUniform1f(glGetUniformLocation(shader, "sky"), 0.0f);
        glUniform1f(glGetUniformLocation(shader, "distancePass"), 0.0f);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        for (int i=0;i<m_cubeCount;i++) {
            m_listCube[i].render();
        }
        glDisable(GL_DEPTH_TEST);
    };
    return m_shadowMap.create(shader, m_listSpotLight, render);
}

void BaseScene::customRender(float width, float height)
{
    glUseProgram(m_shader.generalShader() );
    Matrix4x4 viewMatrix;
    Matrix4x4 mvp;

    if (m_saveImagesRequested) {
        m_saveImagesRequested = false;
        saveCameraImages(m_saveDataRequested);
        m_saveDataRequested = false;
    }
    if (m_cameraIndex > 0) {
        // camera object view uses the lens of the camera object and the aspect ratio of its resolution
        const CameraObject &cameraObject = m_listCameraObject[m_cameraIndex-1];
        const float aspectRatio = cameraObject.getAspectRatio();
        float viewportWidth = width;
        float viewportHeight = height;
        if (width/height > aspectRatio) {
            viewportWidth = height*aspectRatio;
        } else {
            viewportHeight = width/aspectRatio;
        }
        glViewport(static_cast<GLint>((width-viewportWidth)/2.0f), static_cast<GLint>((height-viewportHeight)/2.0f),
                   static_cast<GLsizei>(viewportWidth), static_cast<GLsizei>(viewportHeight));
        mvp = cameraObject.getProjectionMatrix(CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    } else {
        Math3D::MatrixPerspectiveFovLH(&mvp, CAMERA_FIELD_OF_VIEW, width/height, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE);
    }
    viewMatrix = m_camera.getViewMatrix();

    if (m_hideUnseen && m_visibilityMapChanged) {
        createVisibilityMap();
    }
    m_visibilityMap.setupShader(m_shader.generalShader(), m_hideUnseen);
    // camera object does not see its own cone
    renderScene(mvp, viewMatrix, m_cameraIndex-1, nullptr, false);
    // saved images and the maps are rendered without the visibility
    m_visibilityMap.setupShader(m_shader.generalShader(), false);
    m_visibilityMap.unbind();
    if (m_pickPending) {
        renderPick(mvp, viewMatrix, width, height);
    }
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

/*!
 * \brief BaseScene::renderPick
 * renders the distance from the view position to every pixel of the current viewport like the saved distances
 * (float framebuffer, exact also far away) and reads the distance of the picked pixel
 * \param projection projection matrix of the view
 * \param view view matrix of the view
 * \param width, height size of the window
 */
void BaseScene::renderPick(const Matrix4x4 &projection, const Matrix4x4 &view, float width, float height)
{
    m_pickPending = false;
    m_picked = true;
    m_pickedWindowX = static_cast<int>(m_pickX);
    m_pickedWindowY = static_cast<int>(m_pickY);
    m_pickedCamera = m_cameraIndex;
    m_pickedDistance = PICK_OUTSIDE;
    const GLint windowWidth = static_cast<GLint>(width);
    const GLint windowHeight = static_cast<GLint>(height);
    // opengl rows start from the bottom
    const GLint x = m_pickedWindowX;
    const GLint y = windowHeight - 1 - m_pickedWindowY;
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    // camera object view is only in the middle of the window
    if (x < viewport[0] || y < viewport[1] || x >= viewport[0] + viewport[2] || y >= viewport[1] + viewport[3]
        || windowWidth <= 0 || windowHeight <= 0) {
        return;
    }

    GLint oldFramebuffer, oldPackAlignment, oldClampFragmentColor, oldClampReadColor;
    GLfloat oldClearColor[4];
    GLuint framebuffer, renderbuffers[2];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
    glGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor);
    glGetIntegerv(GL_CLAMP_FRAGMENT_COLOR, &oldClampFragmentColor);
    glGetIntegerv(GL_CLAMP_READ_COLOR, &oldClampReadColor);
    const GLboolean oldBlend = glIsEnabled(GL_BLEND);

    glGenFramebuffers(1, &framebuffer);
    glGenRenderbuffers(2, renderbuffers);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_R32F, windowWidth, windowHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, windowWidth, windowHeight);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffers[0]);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffers[1]);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        TG_ERROR_LOG("Cannot create pick framebuffer", std::to_string(windowWidth) + "x" + std::to_string(windowHeight));
    } else {
        // distances are larger than 1.0, so colors must not be clamped or blended
        glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
        glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
        glDisable(GL_BLEND);
        glClearColor(PICK_SKY, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const VECTOR3 position = m_camera.getPosition();
        // same objects as in the view, camera object does not see its own cone
        renderScene(projection, view, m_cameraIndex-1, &position, false);
        GLfloat distance = PICK_SKY;
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(x, y, 1, 1, GL_RED, GL_FLOAT, &distance);
        m_pickedDistance = distance < 0.0f ? PICK_SKY : distance;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(oldFramebuffer));
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteRenderbuffers(2, renderbuffers);
    glPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);
    glClearColor(oldClearColor[0], oldClearColor[1], oldClearColor[2], oldClearColor[3]);
    glClampColor(GL_CLAMP_FRAGMENT_COLOR, static_cast<GLenum>(oldClampFragmentColor));
    glClampColor(GL_CLAMP_READ_COLOR, static_cast<GLenum>(oldClampReadColor));
    if (oldBlend) {
        glEnable(GL_BLEND);
    }
}

/*!
 * \brief BaseScene::pick
 * the next render finds the distance of this window pixel
 * \param x, y window pixels from the top left corner
 */
void BaseScene::pick(float x, float y)
{
    m_pickPending = true;
    m_pickX = x;
    m_pickY = y;
}

/*!
 * \brief BaseScene::takePicked
 * \return true once after every pick, then getPickedText() has its result
 */
bool BaseScene::takePicked()
{
    const bool picked = m_picked;
    m_picked = false;
    return picked;
}

/*!
 * \brief BaseScene::getPickedText
 * \return "<x>/<y> Distance from camera: <distance>" of the picked window pixel, "Camera <number> " before it in
 * the camera object view, distance is from the position of the view in mm, "Sky" if no object is there or
 * "N/A" outside of the image of the camera object view
 */
std::string BaseScene::getPickedText() const
{
    std::string text = std::to_string(m_pickedWindowX) + "/" + std::to_string(m_pickedWindowY) + " Distance from camera: ";
    if (m_pickedCamera > 0) {
        text = "Camera " + std::to_string(m_pickedCamera) + " " + text;
    }
    if (m_pickedDistance == PICK_OUTSIDE) {
        return text + "N/A";
    }
    if (m_pickedDistance == PICK_SKY) {
        return text + "Sky";
    }
    return text + std::to_string(std::llround(static_cast<double>(m_pickedDistance)*MM_TO_OPENGL_UNIT)) + " mm";
}

/*!
 * \brief BaseScene::createVisibilityMap
 * renders the distance maps of the camera objects like their saved images see the scene: in the resolution of
 * the saved images, without the camera objects and with the direction line if it is visible
 */
void BaseScene::createVisibilityMap()
{
    std::vector<CameraObjectConfig> listCamera;
    for (int i=0;i<m_cameraCount;i++) {
        listCamera.push_back(CameraImageSaver::getScaledConfig(m_listCameraObject[i].getConfig(), m_imageOutputScale));
    }
    auto render = [this](const Matrix4x4 &projection, const Matrix4x4 &view, const VECTOR3 &cameraPosition, int) {
        renderScene(projection, view, NO_CAMERA_OBJECTS, &cameraPosition, false);
    };
    if (m_visibilityMap.create(listCamera, render) != 0) {
        TG_ERROR_LOG("Visibility of the cameras cannot be shown");
    }
    m_visibilityMapChanged = false;
}

/*!
 * \brief BaseScene::renderScene
 * renders the scene to the current framebuffer and viewport
 * \param projection projection matrix
 * \param view view matrix
 * \param hiddenCameraObject index of the camera object that is not rendered, -1 renders all (if they are visible),
 * NO_CAMERA_OBJECTS renders none
 * \param distanceFrom if not nullptr, renders distance from this position to every pixel instead
 * of the colors (float framebuffer is needed), sky is not rendered
 * \param surfacePass with distanceFrom: renders the normal toward distanceFrom (rgb) and the surface number
 * (alpha) of every pixel instead of the distance, surface number is SURFACE_FLOOR for the floor and the direction
 * lines, SURFACE_FIRST_CUBE + index for the cubes and then the camera objects
 */
void BaseScene::renderScene(const Matrix4x4 &projection, const Matrix4x4 &view, int hiddenCameraObject,
                            const VECTOR3 *distanceFrom, bool surfacePass)
{
    const int tranformPosition = glGetUniformLocation(m_shader.generalShader(), "transformPosition");
    const int surfaceId = glGetUniformLocation(m_shader.generalShader(), "surfaceId");

    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "projection"), 1, GL_FALSE,  projection.m );
    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "view"), 1, GL_FALSE, view.m );
    glUniform3f(tranformPosition, 0, 0, 0 );
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "earthRadius"), EARTH_RADIUS);
    setupLight();
    m_shadowMap.setupShader(m_shader.generalShader());
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "distancePass"),
                distanceFrom ? (surfacePass ? 2.0f : 1.0f) : 0.0f);
    if (distanceFrom) {
        glUniform3f(glGetUniformLocation(m_shader.generalShader(), "distanceFrom"), distanceFrom->x, distanceFrom->y, distanceFrom->z);
    }
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    if (!distanceFrom) {
        // sky is not bent by the earth curve, spot lights do not light it and
        // its brightness follows the ambient light, which comes from the sky
        glUniform1f(glGetUniformLocation(m_shader.generalShader(), "earthCurve"), 0.0f);
        glUniform1f(glGetUniformLocation(m_shader.generalShader(), "sky"), 1.0f);
        glUniform1f(glGetUniformLocation(m_shader.generalShader(), "skyAmbientRate"), SKY_AMBIENT_RATE);
        m_sky.render();
        glUniform1f(glGetUniformLocation(m_shader.generalShader(), "sky"), 0.0f);
    }
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "earthCurve"), 1.0f);
    glUniform1f(surfaceId, SURFACE_FLOOR);
    m_floor.render();
    if (m_directionLineVisible) {
        m_directionLine.render();
    }
    for (int i=0;i<m_cubeCount;i++) {
        glUniform1f(surfaceId, static_cast<float>(SURFACE_FIRST_CUBE + i));
        m_listCube[i].render();
    }
    if (m_cameraObjectsVisible && hiddenCameraObject != NO_CAMERA_OBJECTS) {
        // camera objects are not bent by the earth curve
        glUniform1f(glGetUniformLocation(m_shader.generalShader(), "earthCurve"), 0.0f);
        for (int i=0;i<m_cameraCount;i++) {
            if (i != hiddenCameraObject) {
                glUniform1f(surfaceId, static_cast<float>(SURFACE_FIRST_CUBE + m_cubeCount + i));
                m_listCameraObject[i].render();
            }
        }
    }
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "distancePass"), 0.0f);
    glDisable(GL_DEPTH_TEST);
}

/*!
 * \brief BaseScene::requestSaveImages
 * images of the camera objects are saved when the next frame is rendered
 * \param withData true saves also the .dat files (key s), false only the png files (key a), if both are requested
 * before the next frame, the .dat files are saved
 */
void BaseScene::requestSaveImages(bool withData)
{
    m_saveImagesRequested = true;
    m_saveDataRequested = m_saveDataRequested || withData;
}

/*!
 * \brief BaseScene::saveCameraImages
 * saves image of every camera object to the output directory as
 * <objects filename>_<timestamp>_<camera number>.png, camera number is 1-cameraCount as in cameraobjects.ini,
 * and with withData the distance of every pixel to <objects filename>_<timestamp>_<camera number>.dat,
 * resolution of the images is imageOutputScale times the resolution of the camera,
 * images smaller than the camera resolution are rendered supersampling times larger and shrunk (anti-aliasing)
 * \param withData true saves also the .dat files, false only the png files
 */
void BaseScene::saveCameraImages(bool withData)
{
    if (m_outputDirectory.empty()) {
        TG_ERROR_LOG("cameraImageOutputDirectory is not set in general.ini, images are not saved");
        return;
    }
    if (!GeneralConfig::createDirectory(m_outputDirectory)) {
        return;
    }

    char timestamp[32];
    const time_t now = time(nullptr);
    struct tm localTime;
    localtime_r(&now, &localTime);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", &localTime);

    // smaller images are rendered larger and shrunk, but never larger than the resolution of the camera
    const int supersampling = std::max(1, std::min(MAX_SUPERSAMPLING, static_cast<int>(floorf(1.0f/m_imageOutputScale + 0.0001f))));
    for (int i=0;i<m_cameraCount;i++) {
        auto render = [this](const Matrix4x4 &projection, const Matrix4x4 &view, const VECTOR3 *distanceFrom,
                                bool surfacePass) {
            // saved images never have the camera objects, even if they are visible in the window
            renderScene(projection, view, NO_CAMERA_OBJECTS, distanceFrom, surfacePass);
        };
        const std::string filename = (std::filesystem::path(m_outputDirectory)
                                      / (std::filesystem::path(m_objectsFile).stem().string() + "_" + timestamp + "_" + std::to_string(i + 1))).string();
        CameraObject camera;
        camera.setup(CameraImageSaver::getScaledConfig(m_listCameraObject[i].getConfig(), m_imageOutputScale), 1.0f);
        if (CameraImageSaver::save(camera, render, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE, supersampling,
                                   i + 1, filename + ".png", withData ? filename + ".dat" : std::string())) {
            TG_INFO_LOG("Saved", filename + (withData ? ".png and .dat" : ".png"));
        }
    }
}

/*!
 * \brief BaseScene::createCameraObjects
 * creates camera objects from cameraObjectsFile of general.ini
 */
void BaseScene::createCameraObjects()
{
    CameraObjectsConfig config;

    if (!config.read(std::string(CONFIGS_PATH) + m_cameraObjectsFile)) {
        return;
    }
    m_cameraCount = config.getCameraCount();
    m_listCameraObject = new CameraObject[m_cameraCount];
    for (int i=0;i<m_cameraCount;i++) {
        m_listCameraObject[i].setup(config.getCamera(i), config.getSizeRate());
    }
}

/*!
 * \brief BaseScene::createObjects
 * creates cubes and spot lights from the objects file
 */
void BaseScene::createObjects()
{
    ObjectsConfig config;

    const bool success = !m_objectsDirectory.empty()
                         && config.read((std::filesystem::path(m_objectsDirectory)/m_objectsFile).string());
    // on failure ambient light is the default, so the floor is still visible
    m_ambientLight = config.getAmbientLight();
    if (!success) {
        return;
    }
    m_cubeCount = config.getCubeCount();
    m_listCube = new Cube[m_cubeCount];
    for (int i=0;i<m_cubeCount;i++) {
        m_listCube[i].setup(config.getCube(i));
    }
    for (int i=0;i<config.getSpotLightCount();i++) {
        if (i == CAMERA_SHADER_MAX_SPOT_LIGHTS) {
            TG_ERROR_LOG("Only", CAMERA_SHADER_MAX_SPOT_LIGHTS, "spot lights are used");
            break;
        }
        m_listSpotLight.push_back(config.getSpotLight(i));
    }
}

void BaseScene::setupLight()
{
    const GLuint shader = m_shader.generalShader();
    const GLsizei count = static_cast<GLsizei>(m_listSpotLight.size());
    std::vector<GLfloat> positions, directions, colors, cosInner, cosOuter;

    for (const SpotLightConfig &light : m_listSpotLight) {
        positions.insert(positions.end(), { light.position.x, light.position.y, light.position.z });
        directions.insert(directions.end(), { light.direction.x, light.direction.y, light.direction.z });
        colors.insert(colors.end(), { light.color.x, light.color.y, light.color.z });
        cosInner.push_back(cosf(light.innerAngle*PI_VALUE/180.0f));
        cosOuter.push_back(cosf(light.outerAngle*PI_VALUE/180.0f));
    }
    glUniform1i(glGetUniformLocation(shader, "lightCount"), count);
    if (count > 0) {
        glUniform3fv(glGetUniformLocation(shader, "lightPosition"), count, positions.data());
        glUniform3fv(glGetUniformLocation(shader, "lightDirection"), count, directions.data());
        glUniform3fv(glGetUniformLocation(shader, "lightColor"), count, colors.data());
        glUniform1fv(glGetUniformLocation(shader, "lightCosInner"), count, cosInner.data());
        glUniform1fv(glGetUniformLocation(shader, "lightCosOuter"), count, cosOuter.data());
    }
    glUniform3f(glGetUniformLocation(shader, "ambientLight"), m_ambientLight.x, m_ambientLight.y, m_ambientLight.z);
}

/*!
 * \brief BaseScene::moveToCamera
 * moves the camera to position and direction of the base camera or camera object,
 * camera object view uses also the roll and the lens of the camera object
 * \param index 0 is the base camera, 1-n is camera object 0-(n-1)
 */
void BaseScene::moveToCamera(int index)
{
    VECTOR3 position, direction, upVector;

    if (index == 0) {
        position = m_basePosition;
        direction = m_baseDirection;
        upVector.x = 0;
        upVector.y = 1.0f;
        upVector.z = 0;
    } else if (index > 0 && index <= m_cameraCount) {
        position = m_listCameraObject[index-1].getPosition();
        direction = m_listCameraObject[index-1].getDirection();
        upVector = m_listCameraObject[index-1].getUpVector();
    } else {
        return;
    }
    m_cameraIndex = index;
    m_camera.setup(position, direction, upVector);
    if (m_yawDirection != 0 || m_pitchDirection != 0) {
        // keeps turning from the new camera
        startTurnFromCurrent();
    }
}

/*!
 * \brief BaseScene::startTurn
 * starts turning (yaw) the current camera around the vertical axis, full circle takes moveSpeed*10 milliseconds
 * \param direction -1 turns to the left, 1 turns to the right
 */
void BaseScene::startTurn(int direction)
{
    setTurnDirection(&m_yawDirection, direction);
}

/*!
 * \brief BaseScene::stopTurn
 * stops turning (yaw), the camera stays in the direction it has turned so far
 * \param direction -1 left or 1 right, only the same direction stops turning
 */
void BaseScene::stopTurn(int direction)
{
    if (direction == m_yawDirection) {
        setTurnDirection(&m_yawDirection, 0);
    }
}

/*!
 * \brief BaseScene::startPitch
 * starts turning (pitch) the current camera up or down, full circle takes moveSpeed*10 milliseconds
 * \param direction -1 turns down, 1 turns up
 */
void BaseScene::startPitch(int direction)
{
    setTurnDirection(&m_pitchDirection, direction);
}

/*!
 * \brief BaseScene::stopPitch
 * stops turning (pitch), the camera stays in the direction it has turned so far
 * \param direction -1 down or 1 up, only the same direction stops turning
 */
void BaseScene::stopPitch(int direction)
{
    if (direction == m_pitchDirection) {
        setTurnDirection(&m_pitchDirection, 0);
    }
}

/*!
 * \brief BaseScene::moveCamera
 * moves the current camera forward or backward to its direction, direction does not change
 * \param steps mouse wheel steps, every step moves moveSpeedMouseWheel mm, positive is forward
 */
void BaseScene::moveCamera(float steps)
{
    const VECTOR3 direction = Math3D::Vec3Normalize(m_camera.getDirection());
    const float distance = steps*m_moveSpeedMouseWheel/MM_TO_OPENGL_UNIT;
    VECTOR3 position = m_camera.getPosition();
    position.x += direction.x*distance;
    position.y += direction.y*distance;
    position.z += direction.z*distance;
    m_camera.setNewPosition(position);
}

/*!
 * \brief BaseScene::setTurnDirection
 * keeps the turn so far and starts turning again from the current camera with the new direction
 * \param turnDirection m_yawDirection or m_pitchDirection
 * \param direction new direction, 0 stops turning
 */
void BaseScene::setTurnDirection(int *turnDirection, int direction)
{
    if (*turnDirection == direction) {
        return;
    }
    updateTurn();
    *turnDirection = direction;
    startTurnFromCurrent();
}

/*!
 * \brief BaseScene::updateTurn
 * sets the camera direction by the time from the start of turning, this is called before every render.
 * Pitch turns the camera around its own right axis, then yaw turns it around the vertical axis.
 * \return true if the camera is turning
 */
bool BaseScene::updateTurn()
{
    if (m_yawDirection == 0 && m_pitchDirection == 0) {
        return false;
    }
    const std::chrono::duration<float, std::milli> elapsed = std::chrono::steady_clock::now() - m_turnStartTime;
    // full circle in moveSpeed*10 milliseconds
    const float angle = elapsed.count()/(m_moveSpeed*10.0f)*2.0f*PI_VALUE;
    // positive yaw turns to the right, positive pitch turns up
    const float yaw = static_cast<float>(m_yawDirection)*angle;
    const float pitch = static_cast<float>(m_pitchDirection)*angle;

    // pitch in the plane of the direction and the up vector
    VECTOR3 direction, upVector;
    direction.x = m_turnStartDirection.x*cosf(pitch) + m_turnStartUpVector.x*sinf(pitch);
    direction.y = m_turnStartDirection.y*cosf(pitch) + m_turnStartUpVector.y*sinf(pitch);
    direction.z = m_turnStartDirection.z*cosf(pitch) + m_turnStartUpVector.z*sinf(pitch);
    upVector.x = m_turnStartUpVector.x*cosf(pitch) - m_turnStartDirection.x*sinf(pitch);
    upVector.y = m_turnStartUpVector.y*cosf(pitch) - m_turnStartDirection.y*sinf(pitch);
    upVector.z = m_turnStartUpVector.z*cosf(pitch) - m_turnStartDirection.z*sinf(pitch);

    auto turnYaw = [yaw](const VECTOR3 &v) {
        VECTOR3 turned;
        turned.x = v.x*cosf(yaw) + v.z*sinf(yaw);
        turned.y = v.y;
        turned.z = -v.x*sinf(yaw) + v.z*cosf(yaw);
        return turned;
    };
    m_camera.setup(m_camera.getPosition(), turnYaw(direction), turnYaw(upVector));
    return true;
}

/*!
 * \brief BaseScene::startTurnFromCurrent
 * turning starts now from the current camera direction, up vector is made
 * perpendicular to the direction, so pitch keeps them perpendicular
 */
void BaseScene::startTurnFromCurrent()
{
    m_turnStartTime = std::chrono::steady_clock::now();
    m_turnStartDirection = Math3D::Vec3Normalize(m_camera.getDirection());
    const VECTOR3 upVector = m_camera.getUpVector();
    const float dot = Math3D::Vec3Dot(upVector, m_turnStartDirection);
    VECTOR3 perpendicular;
    perpendicular.x = upVector.x - m_turnStartDirection.x*dot;
    perpendicular.y = upVector.y - m_turnStartDirection.y*dot;
    perpendicular.z = upVector.z - m_turnStartDirection.z*dot;
    m_turnStartUpVector = Math3D::Vec3Normalize(perpendicular);
}

/*!
 * \brief BaseScene::toggleDirectionLine
 * shows the direction line if it is hidden and hides it if it is visible
 */
void BaseScene::toggleDirectionLine()
{
    m_directionLineVisible = !m_directionLineVisible;
    m_visibilityMapChanged = true;
}

/*!
 * \brief BaseScene::toggleCameraObjects
 * shows the camera object cones if they are hidden and hides them if they are visible
 */
void BaseScene::toggleCameraObjects()
{
    m_cameraObjectsVisible = !m_cameraObjectsVisible;
}

/*!
 * \brief BaseScene::getCameraObjectsVisible
 * \return true if the camera objects (cones) are shown
 */
bool BaseScene::getCameraObjectsVisible() const
{
    return m_cameraObjectsVisible;
}

/*!
 * \brief BaseScene::toggleHideUnseen
 * shows the pixels that no camera object sees (surfaces and sky) as black, or shows them normally
 */
void BaseScene::toggleHideUnseen()
{
    m_hideUnseen = !m_hideUnseen;
}

/*!
 * \brief BaseScene::getHideUnseen
 * \return true if the surfaces and the sky that no camera object sees are black
 */
bool BaseScene::getHideUnseen() const
{
    return m_hideUnseen;
}

void BaseScene::setupVisualPosition()
{
    GeneralConfig config;

    // if reading fails, config keeps the default camera
    config.read(CONFIGS_PATH "general.ini");
    m_basePosition = config.getBasePosition();
    m_baseDirection = config.getBaseDirection();
    m_directionLine.setup(config.getDirectionLineWidth(), config.getDirectionLineHeight());
    m_outputDirectory = config.getOutputDirectory();
    m_objectsDirectory = config.getObjectsDirectory();
    m_moveSpeed = config.getMoveSpeed();
    m_moveSpeedMouseWheel = config.getMoveSpeedMouseWheel();
    m_cameraObjectsFile = config.getCameraObjectsFile();
    // objects file of the command line is used instead of general.ini
    if (m_objectsFile.empty()) {
        m_objectsFile = config.getObjectsFile();
    }
    m_cameraObjectsVisible = config.getCamerasVisible();
    m_imageOutputScale = config.getImageOutputScale();
    moveToCamera(0);
}
