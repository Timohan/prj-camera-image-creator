#include "base_scene.h"
#include "../math/math3d.h"

// vertical field of view, wide enough to see the floor below the camera
#define CAMERA_FIELD_OF_VIEW    (PI_VALUE*2.0f/3.0f)

BaseScene::BaseScene() :
    m_cube(-3.0f, 0.0f, 3.0f, 0.5f, 0.5f, 0.5f)
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
        || m_cube.init()) {
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
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_floor.render();
    m_cube.render();
    glDisable(GL_DEPTH_TEST);
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
