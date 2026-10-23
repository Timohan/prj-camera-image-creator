#include "base_scene.h"
#include "../math/math3d.h"
#include "cameraobjects_config.h"
#include "general_config.h"

// vertical field of view, wide enough to see the floor below the camera
#ifndef CONFIGS_PATH
#define CONFIGS_PATH            "configs/"
#endif

#define CAMERA_FIELD_OF_VIEW    (PI_VALUE*2.0f/3.0f)
#define CAMERA_NEAR_PLANE       0.1f
#define CAMERA_FAR_PLANE        100.0f

// spot light is at 3/3/-3 and points to the middle of the cube
#define SPOT_LIGHT_X            3.0f
#define SPOT_LIGHT_Y            3.0f
#define SPOT_LIGHT_Z            -3.0f
#define SPOT_LIGHT_INNER_ANGLE  (PI_VALUE*25.0f/180.0f)
#define SPOT_LIGHT_OUTER_ANGLE  (PI_VALUE*35.0f/180.0f)
// ambient light is 0.25 of the spot light
#define AMBIENT_LIGHT_FACTOR    0.25f

BaseScene::BaseScene() :
    m_directionLineVisible(true),
    m_cube(-3.0f, 0.0f, 3.0f, 0.5f, 0.5f, 0.5f),
    m_listCameraObject(nullptr),
    m_cameraCount(0),
    m_cameraObjectsVisible(true),
    m_cameraIndex(0)
{
    setupVisualPosition();
    createCameraObjects();
}

BaseScene::~BaseScene()
{
    delete[] m_listCameraObject;
}

int BaseScene::initCustomShader()
{
    if (m_shader.init()) {
        return 1;
    }
    glUseProgram(m_shader.generalShader() );
    if (m_floor.init()
        || m_directionLine.init()
        || m_cube.init()) {
        return 1;
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
    const int tranformPosition = glGetUniformLocation(m_shader.generalShader(), "transformPosition");

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

    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "projection"), 1, GL_FALSE,  mvp.m );
    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "view"), 1, GL_FALSE, viewMatrix.m );
    glUniform3f(tranformPosition, 0, 0, 0 );
    setupLight();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_floor.render();
    if (m_directionLineVisible) {
        m_directionLine.render();
    }
    m_cube.render();
    if (m_cameraObjectsVisible) {
        for (int i=0;i<m_cameraCount;i++) {
            m_listCameraObject[i].render();
        }
    }
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
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

void BaseScene::setupLight()
{
    const GLuint shader = m_shader.generalShader();
    const VECTOR3 cubeCenter = m_cube.getCenter();
    VECTOR3 direction;

    direction.x = cubeCenter.x - SPOT_LIGHT_X;
    direction.y = cubeCenter.y - SPOT_LIGHT_Y;
    direction.z = cubeCenter.z - SPOT_LIGHT_Z;
    direction = Math3D::Vec3Normalize(direction);

    glUniform3f(glGetUniformLocation(shader, "lightPosition"), SPOT_LIGHT_X, SPOT_LIGHT_Y, SPOT_LIGHT_Z);
    glUniform3f(glGetUniformLocation(shader, "lightDirection"), direction.x, direction.y, direction.z);
    glUniform3f(glGetUniformLocation(shader, "lightColor"), 1.0f, 1.0f, 1.0f);
    glUniform1f(glGetUniformLocation(shader, "lightCosInner"), cosf(SPOT_LIGHT_INNER_ANGLE));
    glUniform1f(glGetUniformLocation(shader, "lightCosOuter"), cosf(SPOT_LIGHT_OUTER_ANGLE));
    glUniform1f(glGetUniformLocation(shader, "ambientFactor"), AMBIENT_LIGHT_FACTOR);
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
    moveToCamera(0);
}
