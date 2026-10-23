//
// Created by timo on 20.4.2021.
//

#ifndef CAMERA_H
#define CAMERA_H

#include "math3d.h"

typedef enum
{
    CAMERA_LOOK_AT_AHEAD = 0,
    CAMERA_LOOK_AT_LEFT,
    CAMERA_LOOK_AT_RIGHT,
    CAMERA_LOOK_AT_UP,
    CAMERA_LOOK_AT_BACK,
    CAMERA_LOOK_AT_POV,
} enumCameraLookAtView;

class Camera {
public:
    Camera();
    void setup(VECTOR3 vPosition, VECTOR3 vDirection, VECTOR3 vUpVector);
    void fixEyePosition();
    void setNewPosition(VECTOR3 vPosition);
    VECTOR3 getPosition();
    VECTOR3 getDirection();
    VECTOR3 getUpVector();
    Matrix4x4 getViewMatrix();
    void move(VECTOR3 v);
    void moveUp(float vMove);
    void rotate(float x, float y);
    void moveForward(float fSpeed);
    void saveCamera();
    bool loadLevel(float positionX, float positionY, float positionZ,
                   float directionX, float directionY, float directionZ,
                   float upVectorX, float upVectorY, float upVectorZ);
    float loadSaveGamePositionX();
    float loadSaveGamePositionY();
    float loadSaveGamePositionZ();
    float loadSaveGameDirectionX();
    float loadSaveGameDirectionY();
    float loadSaveGameDirectionZ();
    float loadSaveGameUpVectorX();
    float loadSaveGameUpVectorY();
    float loadSaveGameUpVectorZ();

private:
    VECTOR3                 m_vPosition;
    VECTOR3                 m_vDirection;
    VECTOR3                 m_vUpVector;
};


#endif //CCAMERA_H
