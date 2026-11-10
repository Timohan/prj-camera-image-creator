#include "general_config.h"
#include "ini_file.h"
#include "global_log.h"
#include <filesystem>

#define MM_TO_OPENGL_UNIT   1000.0f
// subdirectories of cameraImageOutputDirectory: saved camera images and objects files
#define CAMERA_OUTPUT_SUBDIRECTORY  "camera_output"
#define CAMERA_OBJECTS_SUBDIRECTORY "camera_objects"

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
        TG_ERROR_LOG(ini.getFilename() + ":", key, "is not positive, using", *value);
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
    m_imageOutputScale = 1.0f;
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
    std::string cameraImageOutputDirectory;
    if (!ini.getString("cameraImageOutputDirectory", &cameraImageOutputDirectory) || cameraImageOutputDirectory.empty()) {
        TG_ERROR_LOG(filename + ": cameraImageOutputDirectory is missing, camera images cannot be saved and objects "
                     "cannot be read");
    } else {
        m_outputDirectory = (std::filesystem::path(cameraImageOutputDirectory) / CAMERA_OUTPUT_SUBDIRECTORY).string();
        m_objectsDirectory = (std::filesystem::path(cameraImageOutputDirectory) / CAMERA_OBJECTS_SUBDIRECTORY).string();
    }
    readPositive(ini, "moveSpeed", &m_moveSpeed);
    readPositive(ini, "moveSpeedMouseWheel", &m_moveSpeedMouseWheel);
    readPositive(ini, "imageOutputScale", &m_imageOutputScale);
    if (ini.hasKey("cameraObjectsFile")
        && (!ini.getString("cameraObjectsFile", &m_cameraObjectsFile) || m_cameraObjectsFile.empty())) {
        m_cameraObjectsFile = "cameraobjects.ini";
        TG_ERROR_LOG(filename + ": cameraObjectsFile is empty, using", m_cameraObjectsFile);
    }
    if (ini.hasKey("objectsFile")
        && (!ini.getString("objectsFile", &m_objectsFile) || m_objectsFile.empty())) {
        m_objectsFile = "objects0.dat";
        TG_ERROR_LOG(filename + ": objectsFile is empty, using", m_objectsFile);
    }
    if (ini.hasKey("camerasVisible")) {
        float visible;
        if (ini.getFloat("camerasVisible", &visible)) {
            m_camerasVisible = visible != 0.0f;
        } else {
            TG_ERROR_LOG(filename + ": camerasVisible is not 0 or 1, using", (m_camerasVisible ? 1 : 0));
        }
    }
    if (!ini.getVector("baseObjectPosition", &position)) {
        TG_ERROR_LOG(filename + ": baseObjectPosition is missing or invalid");
        return false;
    }
    if (!ini.getVector("baseObjectDirection", &direction)
        || (direction.x == 0.0f && direction.y == 0.0f && direction.z == 0.0f)) {
        TG_ERROR_LOG(filename + ": baseObjectDirection is missing or invalid");
        return false;
    }
    if (!ini.getFloat("directionLineWidth", &lineWidth) || lineWidth <= 0.0f) {
        TG_ERROR_LOG(filename + ": directionLineWidth is missing or not positive");
        return false;
    }
    if (!ini.getFloat("directionLinePositionHeight", &lineHeight)) {
        TG_ERROR_LOG(filename + ": directionLinePositionHeight is missing or invalid");
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

/*!
 * \brief GeneralConfig::createDirectory
 * creates the directory and its parents if they do not exist
 * \param directory directory to create
 * \return true if the directory exists now, on failure prints "Could not create directory <directory>"
 */
bool GeneralConfig::createDirectory(const std::string &directory)
{
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error || !std::filesystem::is_directory(directory, error)) {
        TG_ERROR_LOG("Could not create directory", directory);
        return false;
    }
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

const std::string &GeneralConfig::getObjectsDirectory() const
{
    return m_objectsDirectory;
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

float GeneralConfig::getImageOutputScale() const
{
    return m_imageOutputScale;
}
