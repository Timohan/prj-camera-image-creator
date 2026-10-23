#ifndef BASE_SCENE_H
#define BASE_SCENE_H

#include "../shader/camera_shader.h"
#include "../math/camera.h"
#include "floor.h"
#include "cube.h"

class BaseScene
{
public:
    BaseScene();
    ~BaseScene();

    int initCustomShader();
    void customRender(float width, float height);

private:
    CameraShader m_shader;
    Camera m_camera;
    Floor m_floor;
    Cube m_cube;

    void setupVisualPosition();
};

#endif // BASE_SCENE_H
