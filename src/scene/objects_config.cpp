#include "objects_config.h"
#include "ini_file.h"
#include <iostream>

#define MM_TO_OPENGL_UNIT   1000.0f
// x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll before the texture
#define CUBE_VALUE_COUNT    9
// x/y/z/directionX/directionY/directionZ/innerAngle/outerAngle/r/g/b
#define SPOT_LIGHT_VALUE_COUNT  11

/*!
 * \brief ObjectsConfig::read
 * \param filename full path of the objects file
 * \return true on success, on failure the cube and spot light lists are empty
 */
bool ObjectsConfig::read(const std::string &filename)
{
    IniFile ini;
    std::vector<CubeConfig> listCube;
    std::vector<SpotLightConfig> listSpotLight;

    m_listCube.clear();
    m_listSpotLight.clear();
    if (!ini.read(filename)) {
        return false;
    }
    for (const std::string &value : ini.getValues("Cube")) {
        CubeConfig cube;
        float values[CUBE_VALUE_COUNT];
        const size_t textureBegin = value.rfind('/');

        if (textureBegin == std::string::npos
            || !IniFile::parseFloats(value.substr(0, textureBegin), values, CUBE_VALUE_COUNT)) {
            std::cerr << filename << ": Cube=" << value << " is invalid" << std::endl;
            return false;
        }
        cube.texture = value.substr(textureBegin + 1);
        if (values[3] <= 0.0f || values[4] <= 0.0f || values[5] <= 0.0f || cube.texture.empty()) {
            std::cerr << filename << ": Cube=" << value << " must have positive size and texture" << std::endl;
            return false;
        }
        cube.position = { values[0]/MM_TO_OPENGL_UNIT, values[1]/MM_TO_OPENGL_UNIT, values[2]/MM_TO_OPENGL_UNIT };
        cube.size = { values[3]/MM_TO_OPENGL_UNIT, values[4]/MM_TO_OPENGL_UNIT, values[5]/MM_TO_OPENGL_UNIT };
        cube.yaw = values[6];
        cube.pitch = values[7];
        cube.roll = values[8];
        listCube.push_back(cube);
    }
    for (const std::string &value : ini.getValues("SpotLight")) {
        SpotLightConfig light;
        float values[SPOT_LIGHT_VALUE_COUNT];

        if (!IniFile::parseFloats(value, values, SPOT_LIGHT_VALUE_COUNT)) {
            std::cerr << filename << ": SpotLight=" << value << " is invalid" << std::endl;
            return false;
        }
        light.position = { values[0]/MM_TO_OPENGL_UNIT, values[1]/MM_TO_OPENGL_UNIT, values[2]/MM_TO_OPENGL_UNIT };
        light.direction = { values[3], values[4], values[5] };
        light.innerAngle = values[6];
        light.outerAngle = values[7];
        light.color = { values[8], values[9], values[10] };
        if ((light.direction.x == 0.0f && light.direction.y == 0.0f && light.direction.z == 0.0f)
            || light.innerAngle < 0.0f || light.innerAngle >= light.outerAngle || light.outerAngle > 180.0f) {
            std::cerr << filename << ": SpotLight=" << value
                      << " must have direction and 0 <= innerAngle < outerAngle <= 180" << std::endl;
            return false;
        }
        light.direction = Math3D::Vec3Normalize(light.direction);
        listSpotLight.push_back(light);
    }
    if (ini.hasKey("AmbientLight") && !ini.getVector("AmbientLight", &m_ambientLight)) {
        std::cerr << filename << ": AmbientLight is invalid" << std::endl;
        return false;
    }
    m_listCube = listCube;
    m_listSpotLight = listSpotLight;
    return true;
}

int ObjectsConfig::getCubeCount() const
{
    return static_cast<int>(m_listCube.size());
}

const CubeConfig &ObjectsConfig::getCube(int index) const
{
    return m_listCube.at(static_cast<size_t>(index));
}

int ObjectsConfig::getSpotLightCount() const
{
    return static_cast<int>(m_listSpotLight.size());
}

const SpotLightConfig &ObjectsConfig::getSpotLight(int index) const
{
    return m_listSpotLight.at(static_cast<size_t>(index));
}

VECTOR3 ObjectsConfig::getAmbientLight() const
{
    return m_ambientLight;
}
