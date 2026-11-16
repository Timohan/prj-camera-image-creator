#ifndef MODEL_MESH_H
#define MODEL_MESH_H

#include <GL/glew.h>
#include <GL/gl.h>
#include "mesh_tools.h"
#include "objects_config.h"
#include <cstdint>
#include <string>
#include <vector>

/*!
 * \brief ModelMesh
 * a car of a 3D model file (loaded with assimp) in the real size of its measure file, for the object shapes of the
 * models (model_mesh.cpp MODEL_FILES, for example ObjectShape::AudiR8: <modelsDirectory>/car/audi/sport/R8.creator_obj and
 * R8.dat, ObjectShape::ToyotaFortuner: car/toyota/suv/Fortuner.creator_obj and Fortuner.dat).
 * The model file is an .own_obj or .creator_obj file (the own format of the creator, model_mesh.cpp writeOwnObj(), made by
 * convert(), for example of a model simplified by tools/simplify_model.py) or a file that assimp reads (obj...).
 * The model file is in any unit, y up, the wheels on the bottom and the front toward +z. The measure file is
 * key=value lines (# comments), Length=<mm> is the real length of the car, the model is scaled so that its length
 * (z) is this. The vertices are in opengl units in the box of the car (the center of the box is 0/0/0, x is the
 * width, y the height and z the length, the front is +z and the wheels touch the bottom of the box) like the
 * generated cars of car_mesh.h, mirrored in x because the model files are right-handed and the scene is
 * left-handed (the steering wheel stays on its side).
 * The paint parts of the model (part or material names of the model) are the paint triangles (white, they get the
 * texture of the object). The glass parts and the materials that are transparent (d < 1 of the .mtl file) are the
 * glass triangles: transparent in the color images (their color and alpha), solid in the distances and the surface
 * numbers of the data files. The other parts are the detail triangles with the color of their material (the .mtl
 * file of an obj file), some parts have their own colors (model_mesh.cpp OWN_COLORS), materials with a texture image
 * and models without the material file get the colors of their part and material names (tires, rims, black...).
 * Every model is loaded once per process and drawn from one vertex buffer by all of its cars (the shader turns and
 * moves it). The first load of a large model file of assimp takes long (about 2 minutes for the 6.3 million
 * triangles of Fortuner.obj), so the mesh is saved to the cache directory (~/.cache/prj-camera-image-creator, the file name has the
 * model name and a hash of the paths, sizes and change times of the model, material and measure files) and the next
 * processes read it from there. A changed model file gives a new cache file, the old one is removed. An .own_obj
 * file is read fast, it has no cache file.
 */
class ModelMesh
{
public:
    // the mesh of the shape, loaded from modelsDirectory at the first call (later calls can give an empty
    // modelsDirectory), nullptr if the shape has no model or loading fails (the error is printed)
    static const ModelMesh *get(ObjectShape shape, const std::string &modelsDirectory);
    // true if the shape is drawn from a model file
    static bool isModelShape(ObjectShape shape);
    // key of the shape in the objects files, for example "Car_Audi_R8"
    static const char *getObjectsFileKey(ObjectShape shape);
    // all shapes of the model files
    static std::vector<ObjectShape> getModelShapes();
    // reads a model file (obj... with assimp) and writes it as an .own_obj file, 0 on success
    static int convert(const std::string &modelFile, const std::string &ownObjFile);

    // width, height and length of the box of the car in opengl units
    const VECTOR3 &getSize() const;
    bool hasGlass() const;
    // creates the vertex buffer at the first call (needs the opengl context and the shader), 0 on success
    int upload() const;
    // draws the paint (paintTexture) and the details (detailTexture, white), with opaqueGlass also the glass as solid
    void drawOpaque(bool opaqueGlass, GLuint paintTexture, GLuint detailTexture) const;
    // draws the glass (detailTexture, white)
    void drawGlass(GLuint detailTexture) const;

private:
    VECTOR3 m_size = { 0.0f, 0.0f, 0.0f };
    std::vector<MeshVertex> m_vertices;
    // triangles: the paint first, then the details and the glass
    std::vector<uint32_t> m_indices;
    uint32_t m_paintIndexCount = 0;
    uint32_t m_detailIndexCount = 0;
    uint32_t m_glassIndexCount = 0;
    mutable GLuint m_vertexArrayObject = 0;
    mutable GLuint m_vertexBufferObject = 0;
    mutable GLuint m_indexBufferObject = 0;

    bool load(const std::string &modelFile, const std::string &measureFile, const char *const *paintParts,
              const char *const *glassParts);
    bool readCache(const std::string &filename);
    void writeCache(const std::string &filename) const;
    void drawRange(uint32_t first, uint32_t count) const;
};

#endif // MODEL_MESH_H
