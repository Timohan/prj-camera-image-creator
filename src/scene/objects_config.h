#ifndef OBJECTS_CONFIG_H
#define OBJECTS_CONFIG_H

#include <string>
#include <vector>
#include "../math/math3d.h"

// shapes of the objects, a ball and a cylinder fit into the cube of the same size
enum class ObjectShape
{
    Cube,       // box
    Ball,       // ellipsoid, the sizes are its diameters
    Cylinder,   // its axis is the y axis of the cube, size y is its height and sizes x/z the diameters of its ends
    Cone,       // its axis is the y axis of the cube, sizes x/z are the diameters of its base (bottom, -y) and the tip
                // is at the top (+y) of the cube
    Car,        // sedan car in the cube: sizeX is its width with the side mirrors, sizeY its height and sizeZ its
                // length, the front is +z and the wheels touch the bottom of the cube (-y)
    Suv,        // SUV car in the cube like the car: tall body with a steep tailgate, high ground clearance, roof rails
    SportCar,   // sports car in the cube like the car: low 2-door coupe with a long hood, fastback and a rear wing
    Man,        // standing man that fills the cube: sizeX is his width from hand to hand, sizeY his height and sizeZ
                // his depth, he looks toward +z and stands on the bottom of the cube (-y)
    Woman,      // standing woman that fills the cube like the man: T-shirt, knee-length skirt and long hair
    Child,      // standing child (about 7 years) that fills the cube like the man: T-shirt, shorts and sneakers
};

struct CubeConfig
{
    ObjectShape shape = ObjectShape::Cube;
    VECTOR3 position = { 0.0f, 0.0f, 0.0f };    // center of the cube (also of a ball, cylinder or cone) in opengl units
    VECTOR3 size = { 1.0f, 1.0f, 1.0f };        // x/y/z size in opengl units
    float yaw = 0.0f;                           // degrees, positive turns the front (+z) to the right (+x)
    float pitch = 0.0f;                         // degrees, positive turns the front up
    float roll = 0.0f;                          // degrees, positive tilts the top to the right
    std::string texture;                        // image file name in the images folder
};

struct SpotLightConfig
{
    VECTOR3 position = { 0.0f, 0.0f, 0.0f };    // in opengl units
    VECTOR3 direction = { 0.0f, -1.0f, 0.0f };  // normalized
    float innerAngle = 25.0f;                   // degrees from the direction, full light inside
    float outerAngle = 35.0f;                   // degrees from the direction, no light outside
    VECTOR3 color = { 1.0f, 1.0f, 1.0f };       // r/g/b
};

/*!
 * \brief ObjectsConfig
 * Reads objects .dat file: key=value lines, lines starting with # are comments.
 * Every cube is written as Cube=x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll/texture,
 * position and size are in mm (2000 = 2.0 in opengl units), angles in degrees.
 * Every ball is written as Ball=x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll/texture with the same values: the ball is
 * the ellipsoid that fits into the cube of the same values (sizeX/sizeY/sizeZ are its diameters).
 * Every cylinder is written as Cylinder=x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll/texture with the same values: the
 * cylinder fits into the cube of the same values, its axis is the y axis of the cube (sizeY is its height) and its
 * ends are the ellipses of the diameters sizeX and sizeZ.
 * Every cone is written as Cone=x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll/texture with the same values: the cone fits
 * into the cube of the same values, its base is the ellipse of the diameters sizeX and sizeZ at the bottom of the
 * cube (-y) and its tip is at the middle of the top of the cube (+y), sizeY is its height.
 * Every car is written as Car=x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll/texture with the same values: a sedan car fits
 * into the cube of the same values, sizeX is its width (with the side mirrors), sizeY its height and sizeZ its length,
 * its front is +z of the cube and its wheels touch the bottom of the cube (-y). The texture is its paint, the
 * windows, tires, rims, lights, grille and license plates have their own colors.
 * Every SUV is written as Suv=... and every sports car as SportCar=... with the same values as the car.
 * Every man is written as Man=x/y/z/sizeX/sizeY/sizeZ/yaw/pitch/roll/texture with the same values: a standing man
 * (his arms down at his sides) fills the cube of the same values, sizeX is his width from hand to hand, sizeY his
 * height and sizeZ his depth (from his back to his toes), he looks toward +z of the cube and stands on its bottom
 * (-y). The texture is his shirt, his skin, hair, jeans and shoes have their own colors.
 * Every woman (1.65 m, 490/1650/300 keeps her proportions) is written as Woman=... and every child (1.20 m,
 * 360/1200/215) as Child=... with the same values as the man (560/1800/310), the texture is the shirt.
 * The list has the cubes first, then the balls, the cylinders, the cones, the cars, the SUVs, the sports cars, the
 * men, the women and the children.
 * Every spot light is written as
 * SpotLight=x/y/z/directionX/directionY/directionZ/innerAngle/outerAngle/r/g/b,
 * position is in mm, angles in degrees. AmbientLight=r/g/b is optional.
 */
class ObjectsConfig
{
public:
    bool read(const std::string &filename);

    int getCubeCount() const;
    const CubeConfig &getCube(int index) const;
    int getSpotLightCount() const;
    const SpotLightConfig &getSpotLight(int index) const;
    VECTOR3 getAmbientLight() const;

private:
    std::vector<CubeConfig> m_listCube;
    std::vector<SpotLightConfig> m_listSpotLight;
    VECTOR3 m_ambientLight = { 0.25f, 0.25f, 0.25f };
};

#endif // OBJECTS_CONFIG_H
