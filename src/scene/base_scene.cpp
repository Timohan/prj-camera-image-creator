#include "base_scene.h"
#include "../math/math3d.h"
#include "cameraobjects_config.h"
#include "general_config.h"
#include "camera_image_saver.h"
#include <iostream>
#include <filesystem>
#include <time.h>

// vertical field of view, wide enough to see the floor below the camera
#ifndef CONFIGS_PATH
#define CONFIGS_PATH            "configs/"
#endif
#ifndef OBJECTS_PATH
#define OBJECTS_PATH            "objects/"
#endif
// objects file is OBJECTS_PATH OBJECTS_NAME ".dat", saved images start with OBJECTS_NAME
#define OBJECTS_NAME            "objects0"

#define CAMERA_FIELD_OF_VIEW    (PI_VALUE*2.0f/3.0f)
#define MM_TO_OPENGL_UNIT       1000.0f
#define CAMERA_NEAR_PLANE       0.1f
// floor, direction lines and cubes are bent by the curve of the earth
#define EARTH_RADIUS            6371000.0f
// sky brightness is the ambient light multiplied by this
#define SKY_AMBIENT_RATE        2.0f
// far plane reaches the corners of the floor
#define CAMERA_FAR_PLANE        10000.0f

BaseScene::BaseScene() :
    m_directionLineVisible(true),
    m_listCube(nullptr),
    m_cubeCount(0),
    m_ambientLight({ 0.0f, 0.0f, 0.0f }),
    m_listCameraObject(nullptr),
    m_cameraCount(0),
    m_cameraObjectsVisible(true),
    m_cameraIndex(0),
    m_saveImagesRequested(false),
    m_moveSpeed(1000.0f),
    m_moveSpeedMouseWheel(1000.0f),
    m_yawDirection(0),
    m_pitchDirection(0)
{
    setupVisualPosition();
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
    return 0;
}

void BaseScene::customRender(float width, float height)
{
    glUseProgram(m_shader.generalShader() );
    Matrix4x4 viewMatrix;
    Matrix4x4 mvp;

    if (m_saveImagesRequested) {
        m_saveImagesRequested = false;
        saveCameraImages();
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

    // camera object does not see its own cone
    renderScene(mvp, viewMatrix, m_cameraIndex-1);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

/*!
 * \brief BaseScene::renderScene
 * renders the scene to the current framebuffer and viewport
 * \param projection projection matrix
 * \param view view matrix
 * \param hiddenCameraObject index of the camera object that is not rendered, -1 renders all
 */
void BaseScene::renderScene(const Matrix4x4 &projection, const Matrix4x4 &view, int hiddenCameraObject)
{
    const int tranformPosition = glGetUniformLocation(m_shader.generalShader(), "transformPosition");

    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "projection"), 1, GL_FALSE,  projection.m );
    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "view"), 1, GL_FALSE, view.m );
    glUniform3f(tranformPosition, 0, 0, 0 );
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "earthRadius"), EARTH_RADIUS);
    setupLight();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // sky is not bent by the earth curve, spot lights do not light it and
    // its brightness follows the ambient light, which comes from the sky
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "earthCurve"), 0.0f);
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "sky"), 1.0f);
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "skyAmbientRate"), SKY_AMBIENT_RATE);
    m_sky.render();
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "sky"), 0.0f);
    glUniform1f(glGetUniformLocation(m_shader.generalShader(), "earthCurve"), 1.0f);
    m_floor.render();
    if (m_directionLineVisible) {
        m_directionLine.render();
    }
    for (int i=0;i<m_cubeCount;i++) {
        m_listCube[i].render();
    }
    if (m_cameraObjectsVisible) {
        // camera objects are not bent by the earth curve
        glUniform1f(glGetUniformLocation(m_shader.generalShader(), "earthCurve"), 0.0f);
        for (int i=0;i<m_cameraCount;i++) {
            if (i != hiddenCameraObject) {
                m_listCameraObject[i].render();
            }
        }
    }
    glDisable(GL_DEPTH_TEST);
}

/*!
 * \brief BaseScene::requestSaveImages
 * images of the camera objects are saved when the next frame is rendered
 */
void BaseScene::requestSaveImages()
{
    m_saveImagesRequested = true;
}

/*!
 * \brief BaseScene::saveCameraImages
 * saves image of every camera object to the output directory as
 * <objects filename>_<timestamp>_<camera number>.png, camera number is 1-cameraCount as in cameraobjects.ini
 */
void BaseScene::saveCameraImages()
{
    if (m_outputDirectory.empty()) {
        std::cerr << "outputDirectory is not set in general.ini, images are not saved" << std::endl;
        return;
    }
    std::error_code error;
    std::filesystem::create_directories(m_outputDirectory, error);
    if (error) {
        std::cerr << "Cannot create " << m_outputDirectory << ": " << error.message() << std::endl;
        return;
    }

    char timestamp[32];
    const time_t now = time(nullptr);
    struct tm localTime;
    localtime_r(&now, &localTime);
    strftime(timestamp, sizeof(timestamp), "%Y%m%d-%H%M%S", &localTime);

    for (int i=0;i<m_cameraCount;i++) {
        auto render = [this, i](const Matrix4x4 &projection, const Matrix4x4 &view) {
            renderScene(projection, view, i);
        };
        const std::string filename = (std::filesystem::path(m_outputDirectory)
                                      / (std::string(OBJECTS_NAME) + "_" + timestamp + "_" + std::to_string(i + 1) + ".png")).string();
        if (CameraImageSaver::save(m_listCameraObject[i], render, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE, filename)) {
            std::cout << "Saved " << filename << std::endl;
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

    const bool success = config.read(OBJECTS_PATH OBJECTS_NAME ".dat");
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
            std::cerr << "Only " << CAMERA_SHADER_MAX_SPOT_LIGHTS << " spot lights are used" << std::endl;
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
}

/*!
 * \brief BaseScene::toggleCameraObjects
 * shows the camera object cones if they are hidden and hides them if they are visible
 */
void BaseScene::toggleCameraObjects()
{
    m_cameraObjectsVisible = !m_cameraObjectsVisible;
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
    m_moveSpeed = config.getMoveSpeed();
    m_moveSpeedMouseWheel = config.getMoveSpeedMouseWheel();
    m_cameraObjectsFile = config.getCameraObjectsFile();
    moveToCamera(0);
}
