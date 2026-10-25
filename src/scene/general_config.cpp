#include "general_config.h"
#include "ini_file.h"
#include <iostream>

#define MM_TO_OPENGL_UNIT   1000.0f

/*!
 * \brief readPositive
 * reads optional positive value, missing or invalid value keeps the current value
 */
static void readPositive(const IniFile &ini, const std::string &key, float *value)
{
    if (!ini.hasKey(key)) {
        return;
    }
    float newValue;
    if (ini.getFloat(key, &newValue) && newValue > 0.0f) {
        *value = newValue;
    } else {
        std::cerr << ini.getFilename() << ": " << key << " is not positive, using " << *value << std::endl;
    }
}

GeneralConfig::GeneralConfig()
{
    m_basePosition = { 0.0f, 1.0f, 0.0f };
    m_baseDirection = { 0.0f, 0.0f, 1.0f };
    m_directionLineWidth = 0.1f;
    m_directionLineHeight = 0.001f;
    m_moveSpeed = 1000.0f;
    m_moveSpeedMouseWheel = 1000.0f;
    m_cameraObjectsFile = "cameraobjects.ini";
    m_objectsFile = "objects0.dat";
    m_camerasVisible = true;
}

/*!
 * \brief GeneralConfig::read
 * \param filename full path of the ini file
 * \return true on success, on failure the default values are kept
 */
bool GeneralConfig::read(const std::string &filename)
{
    IniFile ini;
    VECTOR3 position, direction;
    float lineWidth, lineHeight;

    if (!ini.read(filename)) {
        return false;
    }
    if (!ini.getString("outputDirectory", &m_outputDirectory) || m_outputDirectory.empty()) {
        std::cerr << filename << ": outputDirectory is missing, camera images cannot be saved" << std::endl;
    }
    readPositive(ini, "moveSpeed", &m_moveSpeed);
    readPositive(ini, "moveSpeedMouseWheel", &m_moveSpeedMouseWheel);
    if (ini.hasKey("cameraObjectsFile")
        && (!ini.getString("cameraObjectsFile", &m_cameraObjectsFile) || m_cameraObjectsFile.empty())) {
        m_cameraObjectsFile = "cameraobjects.ini";
        std::cerr << filename << ": cameraObjectsFile is empty, using " << m_cameraObjectsFile << std::endl;
    }
    if (ini.hasKey("objectsFile")
        && (!ini.getString("objectsFile", &m_objectsFile) || m_objectsFile.empty())) {
        m_objectsFile = "objects0.dat";
        std::cerr << filename << ": objectsFile is empty, using " << m_objectsFile << std::endl;
    }
    if (ini.hasKey("camerasVisible")) {
        float visible;
        if (ini.getFloat("camerasVisible", &visible)) {
            m_camerasVisible = visible != 0.0f;
        } else {
            std::cerr << filename << ": camerasVisible is not 0 or 1, using " << (m_camerasVisible ? 1 : 0) << std::endl;
        }
    }
    if (!ini.getVector("baseObjectPosition", &position)) {
        std::cerr << filename << ": baseObjectPosition is missing or invalid" << std::endl;
        return false;
    }
    if (!ini.getVector("baseObjectDirection", &direction)
        || (direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f)) {
        std::cerr << filename << ": baseObjectDirection is missing or invalid" << std::endl;
        return false;
    }
    if (!ini.getFloat("directionLineWidth", &lineWidth) || lineWidth <= 0.0f) {
        std::cerr << filename << ": directionLineWidth is missing or not positive" << std::endl;
        return false;
    }
    if (!ini.getFloat("directionLinePositionHeight", &lineHeight)) {
        std::cerr << filename << ": directionLinePositionHeight is missing or invalid" << std::endl;
        return false;
    }
    m_directionLineWidth = lineWidth/MM_TO_OPENGL_UNIT;
    m_directionLineHeight = lineHeight/MM_TO_OPENGL_UNIT;
    m_basePosition.x = position.x/MM_TO_OPENGL_UNIT;
    m_basePosition.y = position.y/MM_TO_OPENGL_UNIT;
    m_basePosition.z = position.z/MM_TO_OPENGL_UNIT;
    m_baseDirection = Math3D::Vec3Normalize(direction);
    return true;
}

VECTOR3 GeneralConfig::getBasePosition() const
{
    return m_basePosition;
}

VECTOR3 GeneralConfig::getBaseDirection() const
{
    return m_baseDirection;
}

float GeneralConfig::getDirectionLineWidth() const
{
    return m_directionLineWidth;
}

float GeneralConfig::getDirectionLineHeight() const
{
    return m_directionLineHeight;
}

const std::string &GeneralConfig::getOutputDirectory() const
{
    return m_outputDirectory;
}

float GeneralConfig::getMoveSpeed() const
{
    return m_moveSpeed;
}

float GeneralConfig::getMoveSpeedMouseWheel() const
{
    return m_moveSpeedMouseWheel;
}

const std::string &GeneralConfig::getCameraObjectsFile() const
{
    return m_cameraObjectsFile;
}

const std::string &GeneralConfig::getObjectsFile() const
{
    return m_objectsFile;
}

bool GeneralConfig::getCamerasVisible() const
{
    return m_camerasVisible;
}
