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
#define CAMERA_NEAR_PLANE       0.1f
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
    m_saveImagesRequested(false)
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
    if (m_floor.init()
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

    renderScene(mvp, viewMatrix);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

/*!
 * \brief BaseScene::renderScene
 * renders the scene to the current framebuffer and viewport
 * \param projection projection matrix
 * \param view view matrix
 */
void BaseScene::renderScene(const Matrix4x4 &projection, const Matrix4x4 &view)
{
    const int tranformPosition = glGetUniformLocation(m_shader.generalShader(), "transformPosition");

    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "projection"), 1, GL_FALSE,  projection.m );
    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "view"), 1, GL_FALSE, view.m );
    glUniform3f(tranformPosition, 0, 0, 0 );
    setupLight();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_floor.render();
    if (m_directionLineVisible) {
        m_directionLine.render();
    }
    for (int i=0;i<m_cubeCount;i++) {
        m_listCube[i].render();
    }
    if (m_cameraObjectsVisible) {
        for (int i=0;i<m_cameraCount;i++) {
            m_listCameraObject[i].render();
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
 * <objects filename>_<timestamp>_<camera index>.png
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

    auto render = [this](const Matrix4x4 &projection, const Matrix4x4 &view) {
        renderScene(projection, view);
    };
    for (int i=0;i<m_cameraCount;i++) {
        const std::string filename = (std::filesystem::path(m_outputDirectory)
                                      / (std::string(OBJECTS_NAME) + "_" + timestamp + "_" + std::to_string(i) + ".png")).string();
        if (CameraImageSaver::save(m_listCameraObject[i], render, CAMERA_NEAR_PLANE, CAMERA_FAR_PLANE, filename)) {
            std::cout << "Saved " << filename << std::endl;
        }
    }
}

/*!
 * \brief BaseScene::createCameraObjects
 * creates camera objects from cameraobjects.ini
 */
void BaseScene::createCameraObjects()
{
    CameraObjectsConfig config;

    if (!config.read(CONFIGS_PATH "cameraobjects.ini")) {
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
    moveToCamera(0);
}
