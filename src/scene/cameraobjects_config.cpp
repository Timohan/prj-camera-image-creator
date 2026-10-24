#include "cameraobjects_config.h"
#include "ini_file.h"
#include <iostream>

#define MM_TO_OPENGL_UNIT   1000.0f

/*!
 * \brief readOptional
 * reads count values written as a/b/c..., missing key keeps the values unchanged
 * \return false if the key exists but the values are invalid or not positive when mustBePositive
 */
static bool readOptional(const IniFile &ini, const std::string &key, float *values, int count, bool mustBePositive)
{
    if (!ini.hasKey(key)) {
        return true;
    }
    if (!ini.getFloats(key, values, count)) {
        std::cerr << ini.getFilename() << ": " << key << " is invalid" << std::endl;
        return false;
    }
    for (int i=0;i<count && mustBePositive;i++) {
        if (values[i] <= 0.0f) {
            std::cerr << ini.getFilename() << ": " << key << " must be positive" << std::endl;
            return false;
        }
    }
    return true;
}

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
        const std::string index = std::to_string(i);
        float sensorSize[2] = { camera.sensorWidth, camera.sensorHeight };
        float resolution[2] = { camera.resolutionWidth, camera.resolutionHeight };
        if (!readOptional(ini, "cameraObjectRoll" + index, &camera.roll, 1, false)
            || !readOptional(ini, "cameraObjectFocalLength" + index, &camera.focalLength, 1, true)
            || !readOptional(ini, "cameraObjectSensorSize" + index, sensorSize, 2, true)
            || !readOptional(ini, "cameraObjectResolution" + index, resolution, 2, true)) {
            return false;
        }
        camera.sensorWidth = sensorSize[0];
        camera.sensorHeight = sensorSize[1];
        camera.resolutionWidth = resolution[0];
        camera.resolutionHeight = resolution[1];
        // principal point is in the middle of the image if it is not given
        float principalPoint[2] = { camera.resolutionWidth/2.0f, camera.resolutionHeight/2.0f };
        if (!readOptional(ini, "cameraObjectPrincipalPointOffset" + index, principalPoint, 2, false)) {
            return false;
        }
        camera.principalPointX = principalPoint[0];
        camera.principalPointY = principalPoint[1];
        float focusDistance = camera.focusDistance*MM_TO_OPENGL_UNIT;
        if (!readOptional(ini, "cameraObjectRadialDistortionParameters" + index, camera.radialDistortion, 6, false)
            || !readOptional(ini, "cameraObjectTangentialDistortionParameters" + index, camera.tangentialDistortion, 2, false)
            || !readOptional(ini, "cameraObjectAperture" + index, &camera.aperture, 1, true)
            || !readOptional(ini, "cameraObjectFocusDistance" + index, &focusDistance, 1, true)) {
            return false;
        }
        camera.focusDistance = focusDistance/MM_TO_OPENGL_UNIT;
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
