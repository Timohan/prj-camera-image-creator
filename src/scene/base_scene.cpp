#include "base_scene.h"
#include "../math/math3d.h"

// vertical field of view, wide enough to see the floor below the camera
#define CAMERA_FIELD_OF_VIEW    (PI_VALUE*2.0f/3.0f)

// spot light is at 3/3/-3 and points to the middle of the cube
#define SPOT_LIGHT_X            3.0f
#define SPOT_LIGHT_Y            3.0f
#define SPOT_LIGHT_Z            -3.0f
#define SPOT_LIGHT_INNER_ANGLE  (PI_VALUE*25.0f/180.0f)
#define SPOT_LIGHT_OUTER_ANGLE  (PI_VALUE*35.0f/180.0f)
// ambient light is 0.25 of the spot light
#define AMBIENT_LIGHT_FACTOR    0.25f

static VECTOR3 vector3(float x, float y, float z)
{
    VECTOR3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

BaseScene::BaseScene() :
    m_cube(-3.0f, 0.0f, 3.0f, 0.5f, 0.5f, 0.5f),
    m_cameraObject(vector3(3.0f, 0.5f, 3.0f), m_cube.getCenter(), 0.6f, 0.6f, 0.6f)
{
    setupVisualPosition();
}

BaseScene::~BaseScene()
{
}

int BaseScene::initCustomShader()
{
    if (m_shader.init()) {
        return 1;
    }
    glUseProgram(m_shader.generalShader() );
    if (m_floor.init()
        || m_cube.init()
        || m_cameraObject.init()) {
        return 1;
    }
    return 0;
}

void BaseScene::customRender(float width, float height)
{
    glUseProgram(m_shader.generalShader() );
    Matrix4x4 viewMatrix;
    Matrix4x4 mvp;
    const int tranformPosition = glGetUniformLocation(m_shader.generalShader(), "transformPosition");

    Math3D::MatrixPerspectiveFovLH(&mvp, CAMERA_FIELD_OF_VIEW, width/height, 0.1f, 100.0f);
    viewMatrix = m_camera.getViewMatrix();

    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "projection"), 1, GL_FALSE,  mvp.m );
    glUniformMatrix4fv( glGetUniformLocation(m_shader.generalShader(), "view"), 1, GL_FALSE, viewMatrix.m );
    glUniform3f(tranformPosition, 0, 0, 0 );
    setupLight();
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_floor.render();
    m_cube.render();
    m_cameraObject.render();
    glDisable(GL_DEPTH_TEST);
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

void BaseScene::setupVisualPosition()
{
    VECTOR3 eye, direction, upVector;

    eye.x = 0.0f;
    eye.y = 1.0f;
    eye.z = 0.0f;
    direction.x = 0.0f;
    direction.y = 0.0f;
    direction.z = 1.0f;
    upVector.x = 0;
    upVector.y = 1.0f;
    upVector.z = 0;
    m_camera.setup(eye, direction, upVector);
}
