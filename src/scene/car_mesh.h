#ifndef CAR_MESH_H
#define CAR_MESH_H

#include "mesh_tools.h"

enum class CarType
{
    Sedan,  // 4 doors, hood, cabin and trunk, about 4.6 m x 1.8 m x 1.45 m
    Suv,    // tall 2-box body with a steep tailgate, high ground clearance, large wheels, roof rails and dark
            // side skirts, about 4.7 m x 1.9 m x 1.75 m
    Sport,  // low 2-door coupe with a long hood, a fastback, large low-profile wheels, side intakes and a rear wing,
            // about 4.4 m x 1.95 m x 1.25 m
};

/*!
 * \brief buildCarMesh
 * builds a car that fits into the box of the sizes (the center of the box is 0/0/0, x is the width, y the height
 * and z the length, the front is +z and the wheels touch the bottom of the box): the body (lofted cross-sections
 * with wheel arches, hood, cabin and trunk or tailgate) and the side mirrors are the paint triangles (white, they
 * get the texture of the object), the windows, tires, rims, lights, grille, license plates, door handles and the other details are the
 * detail triangles (their own colors, drawn with a white texture). Every 3 vertices are a triangle.
 * \param sizeX width of the box (with the side mirrors)
 * \param sizeY height of the box
 * \param sizeZ length of the box
 */
void buildCarMesh(CarType type, float sizeX, float sizeY, float sizeZ, std::vector<MeshVertex> &paint,
                  std::vector<MeshVertex> &detail);

#endif // CAR_MESH_H
