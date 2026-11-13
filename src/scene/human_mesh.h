#ifndef HUMAN_MESH_H
#define HUMAN_MESH_H

#include "mesh_tools.h"

enum class HumanType
{
    Man,    // 1.80 m, T-shirt, jeans with a belt, short hair, box 560/1800/310 mm
    Woman,  // 1.65 m, T-shirt, knee-length skirt, long hair to the shoulders, box 490/1650/300 mm
    Child,  // 1.20 m (about 7 years), T-shirt, shorts, sneakers, short hair, box 360/1200/215 mm
};

/*!
 * \brief buildHumanMesh
 * builds a standing person (the arms down at the sides) that fills the box of the sizes: the center of the box is
 * 0/0/0, x is the width (from hand to hand), y the height (the shoes stand on the bottom of the box) and z the depth
 * (from the back to the toes, the person looks toward +z). The person is scaled into the box, so the box sizes of
 * HumanType keep the proportions. The shirt (the torso, the sleeves and the shoulders) is the paint triangles
 * (white, they get the texture of the object), the skin, hair, eyes, jeans, skirt, shorts, belt and shoes are the
 * detail triangles (their own colors, drawn with a white texture). Every 3 vertices are a triangle.
 */
void buildHumanMesh(HumanType type, float sizeX, float sizeY, float sizeZ, std::vector<MeshVertex> &paint,
                    std::vector<MeshVertex> &detail);

#endif // HUMAN_MESH_H
