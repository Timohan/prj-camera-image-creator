#ifndef BASE_SCENE_H
#define BASE_SCENE_H

#include "../shader/camera_shader.h"
#include "../math/camera.h"
#include "floor.h"
#include "sky.h"
#include "cube.h"
#include "direction_line.h"
#include "cameraobject.h"
#include "objects_config.h"
#include "shadow_map.h"
#include <chrono>
#include <string>
#include <vector>

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
    void toggleDirectionLine();
    void toggleCameraObjects();
    void requestSaveImages();
    void startTurn(int direction);
    void stopTurn(int direction);
    void startPitch(int direction);
    void stopPitch(int direction);
    void moveCamera(float steps);
    bool updateTurn();

private:
    CameraShader m_shader;
    Camera m_camera;
    VECTOR3 m_basePosition;
    VECTOR3 m_baseDirection;
    Sky m_sky;
    Floor m_floor;
    DirectionLine m_directionLine;
    bool m_directionLineVisible;
    Cube *m_listCube;
    int m_cubeCount;
    std::vector<SpotLightConfig> m_listSpotLight;
    VECTOR3 m_ambientLight;
    ShadowMap m_shadowMap;
    CameraObject *m_listCameraObject;
    int m_cameraCount;
    bool m_cameraObjectsVisible;
    int m_cameraIndex;
    std::string m_outputDirectory;
    std::string m_cameraObjectsFile;
    bool m_saveImagesRequested;
    float m_moveSpeed;
    float m_moveSpeedMouseWheel;
    // turning with arrow keys, yaw: -1 left, 1 right, pitch: -1 down, 1 up, 0 not turning
    int m_yawDirection;
    int m_pitchDirection;
    std::chrono::steady_clock::time_point m_turnStartTime;
    VECTOR3 m_turnStartDirection;
    VECTOR3 m_turnStartUpVector;

    void createCameraObjects();
    void createObjects();
    void setupLight();
    int createShadowMap();
    void renderScene(const Matrix4x4 &projection, const Matrix4x4 &view, int hiddenCameraObject,
                     const VECTOR3 *distanceFrom);
    void saveCameraImages();
    void startTurnFromCurrent();
    void setTurnDirection(int *turnDirection, int direction);
    void setupVisualPosition();
};

#endif // BASE_SCENE_H
