#include "general_config.h"
#include "ini_file.h"
#include <iostream>

#define MM_TO_OPENGL_UNIT   1000.0f

GeneralConfig::GeneralConfig()
{
    m_basePosition = { 0.0f, 1.0f, 0.0f };
    m_baseDirection = { 0.0f, 0.0f, 1.0f };
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

    if (!ini.read(filename)) {
        return false;
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
