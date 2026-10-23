#ifndef GENERAL_CONFIG_H
#define GENERAL_CONFIG_H

#include <string>
#include "../math/math3d.h"

/*!
 * \brief GeneralConfig
 * Reads general.ini: position and direction of the base camera.
 * Position values in the file are in mm (200 = 0.2 in opengl units).
 * If reading fails, the default values (position 0/1/0, direction 0/0/1) are used.
 */
class GeneralConfig
{
public:
    GeneralConfig();

    bool read(const std::string &filename);

    VECTOR3 getBasePosition() const;
    VECTOR3 getBaseDirection() const;

private:
    VECTOR3 m_basePosition;     // in opengl units
    VECTOR3 m_baseDirection;    // normalized
};

#endif // GENERAL_CONFIG_H
