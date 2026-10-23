#include "cameraobjects_config.h"
#include "ini_file.h"
#include <iostream>

#define MM_TO_OPENGL_UNIT   1000.0f

CameraObjectsConfig::CameraObjectsConfig() :
    m_sizeRate(1.0f)
{
}

/*!
 * \brief CameraObjectsConfig::read
 * \param filename full path of the ini file
 * \return true on success, on failure the camera list is empty
 */
bool CameraObjectsConfig::read(const std::string &filename)
{
    IniFile ini;
    float count;

    m_listCamera.clear();
    if (!ini.read(filename)) {
        return false;
    }
    if (!ini.getFloat("cameraCount", &count)
        || count < 0 || count != static_cast<float>(static_cast<int>(count))) {
        std::cerr << filename << ": cameraCount is missing or not a whole number" << std::endl;
        return false;
    }
    if (!ini.getFloat("cameraSizeRate", &m_sizeRate) || m_sizeRate <= 0.0f) {
        std::cerr << filename << ": cameraSizeRate is missing or not positive" << std::endl;
        return false;
    }

    std::vector<CameraObjectConfig> listCamera;
    for (int i=0;i<static_cast<int>(count);i++) {
        CameraObjectConfig camera;
        if (!ini.getVector("cameraObjectPosition" + std::to_string(i), &camera.position)) {
            std::cerr << filename << ": cameraObjectPosition" << i << " is missing or invalid" << std::endl;
            return false;
        }
        if (!ini.getVector("cameraObjectDirection" + std::to_string(i), &camera.direction)
            || (camera.direction.x == 0.0f && camera.direction.y == 0.0f && camera.direction.z == 0.0f)) {
            std::cerr << filename << ": cameraObjectDirection" << i << " is missing or invalid" << std::endl;
            return false;
        }
        camera.position.x /= MM_TO_OPENGL_UNIT;
        camera.position.y /= MM_TO_OPENGL_UNIT;
        camera.position.z /= MM_TO_OPENGL_UNIT;
        camera.direction = Math3D::Vec3Normalize(camera.direction);
        listCamera.push_back(camera);
    }
    m_listCamera = listCamera;
    return true;
}

int CameraObjectsConfig::getCameraCount() const
{
    return static_cast<int>(m_listCamera.size());
}

float CameraObjectsConfig::getSizeRate() const
{
    return m_sizeRate;
}

const CameraObjectConfig &CameraObjectsConfig::getCamera(int index) const
{
    return m_listCamera.at(static_cast<size_t>(index));
}
