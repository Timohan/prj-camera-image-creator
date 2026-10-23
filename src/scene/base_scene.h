#ifndef BASE_SCENE_H
#define BASE_SCENE_H

#include "../shader/camera_shader.h"
#include "../math/camera.h"
#include "floor.h"
#include "cube.h"
#include "cameraobject.h"

class BaseScene
{
public:
    BaseScene();
    ~BaseScene();
    BaseScene(const BaseScene &) = delete;
    BaseScene &operator=(const BaseScene &) = delete;

    int initCustomShader();
    void customRender(float width, float height);
    void moveToCamera(int index);

private:
    CameraShader m_shader;
    Camera m_camera;
    VECTOR3 m_basePosition;
    VECTOR3 m_baseDirection;
    Floor m_floor;
    Cube m_cube;
    CameraObject *m_listCameraObject;
    int m_cameraCount;

    void createCameraObjects();
    void setupLight();
    void setupVisualPosition();
};

#endif // BASE_SCENE_H
