#include "objects_config.h"
#include "ini_file.h"
#include "global_log.h"
#include "model_mesh.h"
#include <utility>

#define MM_TO_OPENGL_UNIT   1000.0f
// x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll before the texture
#define CUBE_VALUE_COUNT    9
// model shapes: x/z/yaw before the texture
#define MODEL_VALUE_COUNT   3
// x/y/z/directionX/directionY/directionZ/innerAngle/outerAngle/r/g/b
#define SPOT_LIGHT_VALUE_COUNT  11

/*!
 * \brief ObjectsConfig::read
 * \param filename full path of the objects file
 * \param modelsDirectory directory of the model files of the model shapes (Car_Audi_R8)
 * \return true on success, on failure the cube and spot light lists are empty
 */
bool ObjectsConfig::read(const std::string &filename, const std::string &modelsDirectory)
{
    IniFile ini;
    std::vector<CubeConfig> listCube;
    std::vector<SpotLightConfig> listSpotLight;

    m_listCube.clear();
    m_listSpotLight.clear();
    if (!ini.read(filename)) {
        return false;
    }
    const std::pair<const char *, ObjectShape> shapes[] = {
        { "Cube", ObjectShape::Cube }, { "Ball", ObjectShape::Ball }, { "Cylinder", ObjectShape::Cylinder },
        { "Cone", ObjectShape::Cone }, { "Car", ObjectShape::Car },
        { "Suv", ObjectShape::Suv }, { "SportCar", ObjectShape::SportCar },
        { "Man", ObjectShape::Man }, { "Woman", ObjectShape::Woman }, { "Child", ObjectShape::Child } };
    for (const auto &[key, shape] : shapes) {
        for (const std::string &value : ini.getValues(key)) {
            CubeConfig cube;
            float values[CUBE_VALUE_COUNT];
            const size_t textureBegin = value.rfind('/');

            if (textureBegin == std::string::npos
                || !IniFile::parseFloats(value.substr(0, textureBegin), values, CUBE_VALUE_COUNT)) {
                TG_ERROR_LOG(filename + ": " + key + "=" + value, "is invalid");
                return false;
            }
            cube.shape = shape;
            cube.texture = value.substr(textureBegin + 1);
            if (values[3] <= 0.0f || values[4] <= 0.0f || values[5] <= 0.0f || cube.texture.empty()) {
                TG_ERROR_LOG(filename + ": " + key + "=" + value, "must have positive size and texture");
                return false;
            }
            cube.position = { values[0]/MM_TO_OPENGL_UNIT, values[1]/MM_TO_OPENGL_UNIT, values[2]/MM_TO_OPENGL_UNIT };
            cube.size = { values[3]/MM_TO_OPENGL_UNIT, values[4]/MM_TO_OPENGL_UNIT, values[5]/MM_TO_OPENGL_UNIT };
            cube.yaw = values[6];
            cube.pitch = values[7];
            cube.roll = values[8];
            listCube.push_back(cube);
        }
    }
    // the model shapes stand on the floor, their cube is the box of the model
    for (const ObjectShape shape : ModelMesh::getModelShapes()) {
        const std::string key = ModelMesh::getObjectsFileKey(shape);
        for (const std::string &value : ini.getValues(key)) {
            CubeConfig cube;
            float values[MODEL_VALUE_COUNT];
            const size_t textureBegin = value.rfind('/');

            if (textureBegin == std::string::npos
                || !IniFile::parseFloats(value.substr(0, textureBegin), values, MODEL_VALUE_COUNT)
                || textureBegin + 1 >= value.size()) {
                TG_ERROR_LOG(filename + ": " + key + "=" + value, "is invalid, it must be x/z/yaw/texture");
                return false;
            }
            const ModelMesh *model = ModelMesh::get(shape, modelsDirectory);
            if (!model) {
                TG_ERROR_LOG(filename + ": " + key + "=" + value, "cannot be drawn, its model is not loaded");
                return false;
            }
            cube.shape = shape;
            cube.texture = value.substr(textureBegin + 1);
            cube.size = model->getSize();
            cube.position = { values[0]/MM_TO_OPENGL_UNIT, cube.size.y/2.0f, values[1]/MM_TO_OPENGL_UNIT };
            cube.yaw = values[2];
            listCube.push_back(cube);
        }
    }
    for (const std::string &value : ini.getValues("SpotLight")) {
        SpotLightConfig light;
        float values[SPOT_LIGHT_VALUE_COUNT];

        if (!IniFile::parseFloats(value, values, SPOT_LIGHT_VALUE_COUNT)) {
            TG_ERROR_LOG(filename + ": SpotLight=" + value, "is invalid");
            return false;
        }
        light.position = { values[0]/MM_TO_OPENGL_UNIT, values[1]/MM_TO_OPENGL_UNIT, values[2]/MM_TO_OPENGL_UNIT };
        light.direction = { values[3], values[4], values[5] };
        light.innerAngle = values[6];
        light.outerAngle = values[7];
        light.color = { values[8], values[9], values[10] };
        if ((light.direction.x == 0.0f && light.direction.y == 0.0f && light.direction.z == 0.0f)
            || light.innerAngle < 0.0f || light.innerAngle >= light.outerAngle || light.outerAngle > 180.0f) {
            TG_ERROR_LOG(filename + ": SpotLight=" + value,
                         "must have direction and 0 <= innerAngle < outerAngle <= 180");
            return false;
        }
        light.direction = Math3D::Vec3Normalize(light.direction);
        listSpotLight.push_back(light);
    }
    if (ini.hasKey("AmbientLight") && !ini.getVector("AmbientLight", &m_ambientLight)) {
        TG_ERROR_LOG(filename + ": AmbientLight is invalid");
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
