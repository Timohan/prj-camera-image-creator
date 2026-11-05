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
#include "visibility_map.h"
#include <chrono>
#include <string>
#include <vector>

class BaseScene
{
public:
    // objectsFile: file in <cameraImageOutputDirectory>/camera_objects of general.ini, if it is empty, objectsFile of general.ini is used,
    // camerasVisible: 0 or 1, if it is -1, camerasVisible of general.ini is used
    explicit BaseScene(const std::string &objectsFile = std::string(), int camerasVisible = -1);
    ~BaseScene();
    BaseScene(const BaseScene &) = delete;
    BaseScene &operator=(const BaseScene &) = delete;

    int initCustomShader();
    void customRender(float width, float height);
    void moveToCamera(int index);
    void toggleDirectionLine();
    void toggleCameraObjects();
    bool getCameraObjectsVisible() const;
    void toggleHideUnseen();
    bool getHideUnseen() const;
    // withData: saves also the .dat files
    void requestSaveImages(bool withData = true);
    void startTurn(int direction);
    void stopTurn(int direction);
    void startPitch(int direction);
    void stopPitch(int direction);
    void moveCamera(float steps);
    bool updateTurn();
    // picks the window pixel (from the top left corner) in the next render
    void pick(float x, float y);
    bool takePicked();
    std::string getPickedText() const;

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
    VisibilityMap m_visibilityMap;
    bool m_hideUnseen;                  // pixels that no camera object sees are black
    bool m_visibilityMapChanged;        // visibility map must be rendered again before it is used
    CameraObject *m_listCameraObject;
    int m_cameraCount;
    bool m_cameraObjectsVisible;
    int m_cameraIndex;
    std::string m_outputDirectory;
    std::string m_cameraObjectsFile;
    std::string m_objectsDirectory;     // <cameraImageOutputDirectory>/camera_objects of general.ini
    std::string m_objectsFile;
    float m_imageOutputScale;
    bool m_saveImagesRequested;
    bool m_saveDataRequested;           // .dat files are saved with the requested images
    float m_moveSpeed;
    float m_moveSpeedMouseWheel;
    // turning with arrow keys, yaw: -1 left, 1 right, pitch: -1 down, 1 up, 0 not turning
    int m_yawDirection;
    int m_pitchDirection;
    std::chrono::steady_clock::time_point m_turnStartTime;
    VECTOR3 m_turnStartDirection;
    VECTOR3 m_turnStartUpVector;
    bool m_pickPending;                 // pick at m_pickX, m_pickY in the next render
    float m_pickX;                      // window pixels from the top left corner
    float m_pickY;
    bool m_picked;                      // a pick was done and takePicked() has not returned it yet
    int m_pickedWindowX;                // picked pixel, window pixels from the top left corner
    int m_pickedWindowY;
    int m_pickedCamera;                 // view of the pick, 0 is the base camera, 1-n is the camera object 1-n
    // distance of the picked pixel from the view position in opengl units, PICK_SKY or PICK_OUTSIDE
    float m_pickedDistance;

    void createCameraObjects();
    void createObjects();
    void setupLight();
    int createShadowMap();
    void createVisibilityMap();
    void renderScene(const Matrix4x4 &projection, const Matrix4x4 &view, int hiddenCameraObject,
                     const VECTOR3 *distanceFrom, bool surfacePass);
    void saveCameraImages(bool withData);
    void startTurnFromCurrent();
    void setTurnDirection(int *turnDirection, int direction);
    void setupVisualPosition();
    void renderPick(const Matrix4x4 &projection, const Matrix4x4 &view, float width, float height);
};

#endif // BASE_SCENE_H
