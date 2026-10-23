#ifndef CAMERA_OBJECTS_CONFIG_H
#define CAMERA_OBJECTS_CONFIG_H

#include <string>
#include <vector>
#include "../math/math3d.h"

struct CameraObjectConfig
{
    VECTOR3 position = { 0.0f, 0.0f, 0.0f };    // in opengl units
    VECTOR3 direction = { 0.0f, 0.0f, 1.0f };   // normalized
    float roll = 0.0f;                          // degrees, positive rolls the camera clockwise seen from behind
    float focalLength = 35.0f;                  // mm
    float sensorWidth = 36.0f;                  // mm
    float sensorHeight = 24.0f;                 // mm
    float resolutionWidth = 1920.0f;            // pixels
    float resolutionHeight = 1080.0f;           // pixels
    float principalPointX = 960.0f;             // pixels from the left edge
    float principalPointY = 540.0f;             // pixels from the top edge
};

/*!
 * \brief CameraObjectsConfig
 * Reads cameraobjects.ini: key=value lines, lines starting with # are comments.
 * Position values in the file are in mm (200 = 0.2 in opengl units).
 * Lens values (roll, focal length, sensor size, resolution, principal point)
 * are optional, missing values use the defaults of CameraObjectConfig.
 */
class CameraObjectsConfig
{
public:
    CameraObjectsConfig();

    bool read(const std::string &filename);

    int getCameraCount() const;
    float getSizeRate() const;
    const CameraObjectConfig &getCamera(int index) const;

private:
    float m_sizeRate;
    std::vector<CameraObjectConfig> m_listCamera;
};

#endif // CAMERA_OBJECTS_CONFIG_H
