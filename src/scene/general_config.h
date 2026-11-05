#ifndef GENERAL_CONFIG_H
#define GENERAL_CONFIG_H

#include <string>
#include "../math/math3d.h"

/*!
 * \brief GeneralConfig
 * Reads general.ini: position and direction of the base camera and
 * width and height position of the direction lines on the floor.
 * Position values in the file are in mm (200 = 0.2 in opengl units).
 * If reading fails, the default values (position 0/1/0, direction 0/0/1, line width 0.1, line height 0.001) are used.
 * cameraImageOutputDirectory is read separately, the output directory of the saved camera images is
 * <cameraImageOutputDirectory>/camera_output and the objects directory is <cameraImageOutputDirectory>/camera_objects,
 * both are empty if cameraImageOutputDirectory is missing.
 * moveSpeed: camera turns full circle in moveSpeed*10 milliseconds,
 * moveSpeedMouseWheel: one mouse wheel step moves the camera moveSpeedMouseWheel mm,
 * they are read separately and they are 1000 if they are missing.
 * cameraObjectsFile is the file of the camera objects in the configs folder,
 * it is read separately and it is cameraobjects.ini if it is missing.
 * objectsFile is the file of the cubes and lights in <cameraImageOutputDirectory>/camera_objects,
 * it is objects0.dat if it is missing.
 * camerasVisible is 1 if the cameras are drawn at the start and 0 if not, it is 1 if it is missing.
 * imageOutputScale scales the resolution of the saved images (0.5 = half width and height), it is 1 if it is missing.
 */
class GeneralConfig
{
public:
    GeneralConfig();

    bool read(const std::string &filename);
    // creates the directory and its parents if they do not exist,
    // prints "Could not create directory <directory>" and returns false on failure
    static bool createDirectory(const std::string &directory);

    VECTOR3 getBasePosition() const;
    VECTOR3 getBaseDirection() const;
    float getDirectionLineWidth() const;
    float getDirectionLineHeight() const;
    const std::string &getOutputDirectory() const;
    float getMoveSpeed() const;
    float getMoveSpeedMouseWheel() const;
    const std::string &getCameraObjectsFile() const;
    const std::string &getObjectsDirectory() const;
    const std::string &getObjectsFile() const;
    bool getCamerasVisible() const;
    float getImageOutputScale() const;

private:
    VECTOR3 m_basePosition;     // in opengl units
    VECTOR3 m_baseDirection;    // normalized
    float m_directionLineWidth;     // in opengl units
    float m_directionLineHeight;    // in opengl units
    std::string m_outputDirectory;     // <cameraImageOutputDirectory>/camera_output
    float m_moveSpeed;              // milliseconds
    float m_moveSpeedMouseWheel;    // mm
    std::string m_cameraObjectsFile;
    std::string m_objectsDirectory;    // <cameraImageOutputDirectory>/camera_objects
    std::string m_objectsFile;
    bool m_camerasVisible;
    float m_imageOutputScale;
};

#endif // GENERAL_CONFIG_H
