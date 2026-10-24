#include "general_config.h"
#include "ini_file.h"
#include <iostream>

#define MM_TO_OPENGL_UNIT   1000.0f

GeneralConfig::GeneralConfig()
{
    m_basePosition = { 0.0f, 1.0f, 0.0f };
    m_baseDirection = { 0.0f, 0.0f, 1.0f };
    m_directionLineWidth = 0.1f;
    m_directionLineHeight = 0.001f;
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
