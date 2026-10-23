
#include "camera.h"

Camera::Camera()
{
VECTOR3 vPosition;
VECTOR3 vDirection;
VECTOR3 vUpVector;

vPosition.x = 0.0f;
vPosition.y = 0.0f;
vPosition.z = 0.001f;

vDirection.x = 0.0f;
vDirection.y = 1.0f;
vDirection.z = 0.001f;

vUpVector.x = 0.0f;
vUpVector.y = 1.0f;
vUpVector.z = 0.0f;

setup(vPosition,vDirection,vUpVector);
}


void Camera::setup(VECTOR3 vPosition, VECTOR3 vDirection, VECTOR3 vUpVector)
{
    m_vPosition = vPosition;
    m_vDirection = vDirection;
    m_vDirection = Math3D::Vec3Normalize(m_vDirection);
}

void Camera::fixEyePosition()
{

}

void Camera::setNewPosition(VECTOR3 vPosition)
{
    m_vPosition = vPosition;
    fixEyePosition();
}

VECTOR3 Camera::getPosition()
{
return m_vPosition;
}

VECTOR3 Camera::getDirection()
{
return m_vDirection;
}

VECTOR3 Camera::getUpVector()
{
return m_vUpVector;
}
Matrix4x4 Camera::getViewMatrix()
{
    Matrix4x4 view;
    VECTOR3 pAt;
    pAt.x = m_vDirection.x + m_vPosition.x;
    pAt.y = m_vDirection.y + m_vPosition.y;
    pAt.z = m_vDirection.z + m_vPosition.z;

    VECTOR3 vUpVector;
    vUpVector.x = 0.0f;
    vUpVector.y = 1.0f;
    vUpVector.z = 0.0f;

    Math3D::MatrixLookAtLH(&view,m_vPosition,pAt,vUpVector);
    return view;
}

void Camera::move(VECTOR3 v)
{
    VECTOR3 vZero;
    Matrix4x4 m;
    VECTOR3 v1;
    vZero.x = 0.0f;
    vZero.y = 0.0f;
    vZero.z = 0.0f;
    float angle1 = Math3D::calcAngleXZ(vZero,m_vDirection);


    Math3D::SetRotateY(&m,angle1);
    v1 = Math3D::vectorMatrix(&v,&m);


    m_vPosition.x += v1.x;
    m_vPosition.y += v1.y;
    m_vPosition.z += v1.z;

    fixEyePosition();
}

void Camera::moveUp(float vMove)
{
    m_vPosition.y += vMove;
    fixEyePosition();
}

void Camera::rotate(float x, float y)
{
    Matrix4x4 mX;

    Math3D::SetRotateY(&mX,x);

    m_vDirection = Math3D::vectorMatrix(&m_vDirection, &mX);

    if (m_vDirection.y + y > 0.95f)
    {
        m_vDirection.y = 0.95f;
    }
    else if (m_vDirection.y + y < -0.95f)
    {
        m_vDirection.y = -0.95f;
    }
    else {
        m_vDirection.y += y;
    }

    m_vDirection = Math3D::Vec3Normalize(m_vDirection);
}

void Camera::moveForward(float fSpeed)
{
    m_vPosition.x += m_vDirection.x*fSpeed;
    m_vPosition.y += m_vDirection.y*fSpeed;
    m_vPosition.z += m_vDirection.z*fSpeed;

    fixEyePosition();
}

void Camera::saveCamera() {
    /*
NSString *strPositionX = [NSString stringWithFormat:@"%f", m_vPosition.x ];
NSString *strstrPositionXIndexKey = [NSString stringWithFormat:@"FindFromMap3DPositionX" ];
[userDefaults setObject:strPositionX forKey:strstrPositionXIndexKey];

NSString *strPositionY = [NSString stringWithFormat:@"%f", m_vPosition.y ];
NSString *strstrPositionYIndexKey = [NSString stringWithFormat:@"FindFromMap3DPositionY" ];
[userDefaults setObject:strPositionY forKey:strstrPositionYIndexKey];

NSString *strPositionZ = [NSString stringWithFormat:@"%f", m_vPosition.z ];
NSString *strstrPositionZIndexKey = [NSString stringWithFormat:@"FindFromMap3DPositionZ" ];
[userDefaults setObject:strPositionZ forKey:strstrPositionZIndexKey];

NSString *strDirectionX = [NSString stringWithFormat:@"%f", m_vDirection.x ];
NSString *strDirectionXIndexKey = [NSString stringWithFormat:@"FindFromMap3DDirectionX" ];
[userDefaults setObject:strDirectionX forKey:strDirectionXIndexKey];

NSString *strDirectionY = [NSString stringWithFormat:@"%f", m_vDirection.y ];
NSString *strDirectionYIndexKey = [NSString stringWithFormat:@"FindFromMap3DDirectionY" ];
[userDefaults setObject:strDirectionY forKey:strDirectionYIndexKey];

NSString *strDirectionZ = [NSString stringWithFormat:@"%f", m_vDirection.z ];
NSString *strDirectionZIndexKey = [NSString stringWithFormat:@"FindFromMap3DDirectionZ" ];
[userDefaults setObject:strDirectionZ forKey:strDirectionZIndexKey];

NSString *strVectorX = [NSString stringWithFormat:@"%f", m_vUpVector.x ];
NSString *strVectorXIndexKey = [NSString stringWithFormat:@"FindFromMap3DVectorX" ];
[userDefaults setObject:strVectorX forKey:strVectorXIndexKey];

NSString *strVectorY = [NSString stringWithFormat:@"%f", m_vUpVector.y ];
NSString *strVectorYIndexKey = [NSString stringWithFormat:@"FindFromMap3DVectorY" ];
[userDefaults setObject:strVectorY forKey:strVectorYIndexKey];

NSString *strVectorZ = [NSString stringWithFormat:@"%f", m_vUpVector.z ];
NSString *strVectorZIndexKey = [NSString stringWithFormat:@"FindFromMap3DVectorZ" ];
[userDefaults setObject:strVectorZ forKey:strVectorZIndexKey];*/
}

float Camera::loadSaveGamePositionX()
{
    return m_vPosition.x;
}
float Camera::loadSaveGamePositionY()
{
    return m_vPosition.y;
}
float Camera::loadSaveGamePositionZ()
{
    return m_vPosition.z;
}
float Camera::loadSaveGameDirectionX()
{
    return m_vDirection.x;
}
float Camera::loadSaveGameDirectionY()
{
    return m_vDirection.y;
}
float Camera::loadSaveGameDirectionZ()
{
    return m_vDirection.z;
}
float Camera::loadSaveGameUpVectorX()
{
    return m_vUpVector.x;
}
float Camera::loadSaveGameUpVectorY()
{
    return m_vUpVector.y;
}
float Camera::loadSaveGameUpVectorZ()
{
    return m_vUpVector.z;
}

bool Camera::loadLevel(float positionX, float positionY, float positionZ,
                        float directionX, float directionY, float directionZ,
                        float upVectorX, float upVectorY, float upVectorZ)
{
    VECTOR3 vPosition;
    VECTOR3 vDirection;
    VECTOR3 vUpVector;

    vPosition.x = positionX;
    vPosition.y = positionY;
    vPosition.z = positionZ;
    vDirection.x = directionX;
    vDirection.y = directionY;
    vDirection.z = directionZ;
    vUpVector.x = upVectorX;
    vUpVector.y = upVectorY;
    vUpVector.z = upVectorZ;

    setup(vPosition,vDirection,vUpVector);
    return true;
}