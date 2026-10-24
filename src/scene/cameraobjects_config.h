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
    bool fisheye = false;                       // false: OpenCV pinhole model, true: OpenCV fisheye model
    float radialDistortion[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };   // k1-k6, pinhole model
    float tangentialDistortion[2] = { 0.0f, 0.0f };                         // p1-p2, pinhole model
    float fisheyeDistortion[4] = { 0.0f, 0.0f, 0.0f, 0.0f };              // k1-k4, fisheye model
    float fieldOfView = 360.0f;                 // degrees, fisheye model: rays wider than this are black
    float aperture = 0.0f;                      // f-number, 0 = no depth of field (pinhole)
    float focusDistance = 3.0f;                 // in opengl units
};

/*!
 * \brief CameraObjectsConfig
 * Reads cameraobjects.ini: key=value lines, lines starting with # are comments.
 * Position values in the file are in mm (200 = 0.2 in opengl units).
 * Lens values (roll, focal length, sensor size, resolution, principal point,
 * distortion model and distortion, field of view, aperture, focus distance)
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
