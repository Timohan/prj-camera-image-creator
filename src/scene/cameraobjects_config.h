#ifndef CAMERA_OBJECTS_CONFIG_H
#define CAMERA_OBJECTS_CONFIG_H

#include <string>
#include <vector>
#include "../math/math3d.h"

struct CameraObjectConfig
{
    VECTOR3 position;   // in opengl units
    VECTOR3 direction;  // normalized
};

/*!
 * \brief CameraObjectsConfig
 * Reads cameraobjects.ini: key=value lines, lines starting with # are comments.
 * Position values in the file are in mm (200 = 0.2 in opengl units).
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
