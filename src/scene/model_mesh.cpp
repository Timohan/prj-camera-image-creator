#include "model_mesh.h"
#include "ini_file.h"
#include "global_log.h"
#include "../shader/camera_shader.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <unistd.h>

#define MM_TO_OPENGL_UNIT   1000.0f
// the cache files of the loaded models, the version changes when the mesh (colors, parts) is made differently
#define MODEL_CACHE_SUBDIRECTORY    "prj-camera-image-creator"
#define MODEL_CACHE_MAGIC           "PCIM"
#define MODEL_CACHE_VERSION         1

namespace {

// the models: shape, key in the objects files, model file and measure file in modelsDirectory, the names of the
// paint parts and of the glass parts (part or material names, lowercase, without the .001 numbers of the copies)
struct ModelFile
{
    ObjectShape shape;
    const char *key;
    const char *modelFile;
    const char *measureFile;
    const char *const *paintParts;
    const char *const *glassParts;
};

const char *const R8_PAINT[] = { "body color", nullptr };
// the lamp glass is transparent like the windows (its material is the same white glass), the lamps are seen through it
const char *const R8_GLASS[] = { "glass window", "glass lamp", nullptr };
const char *const FORTUNER_PAINT[] = { "carpaint", "car_paint", nullptr };
const char *const FORTUNER_GLASS[] = { nullptr };
const char *const SCLASS_PAINT[] = { "w222_-_car_color", nullptr };
// the windows and the headlight glass (the materials have no transparency)
const char *const SCLASS_GLASS[] = { "w222_-_glass_-_tint", "w222_-_glass_-_polarized", nullptr };

const ModelFile MODEL_FILES[] = {
    { ObjectShape::AudiR8, "Car_Audi_R8", "car/audi/sport/R8.creator_obj", "car/audi/sport/R8.dat", R8_PAINT, R8_GLASS },
    { ObjectShape::ToyotaFortuner, "Car_Toyota_Fortuner", "car/toyota/suv/Fortuner.creator_obj",
      "car/toyota/suv/Fortuner.dat", FORTUNER_PAINT, FORTUNER_GLASS },
    { ObjectShape::MercedesSClass, "Car_Mercedes_SClass", "car/mercedes-benz/sedan/SClass.creator_obj",
      "car/mercedes-benz/sedan/SClass.dat", SCLASS_PAINT, SCLASS_GLASS },
};

struct PartColor
{
    const char *name;
    float color[4];
};

// the glass parts that have no transparency in the material file (the R8 windows: a white glass that needs
// transparency): dark tinted glass, the rest of the light shows the interior behind it
#define MODEL_GLASS_ALPHA   0.35f
const float GLASS_COLOR[4] = { 0.10f, 0.12f, 0.14f, MODEL_GLASS_ALPHA };

// parts (lowercase part of the part or material name) that have their own color instead of the material color:
// the black materials that have a white color are black
const PartColor OWN_COLORS[] = {
    { "soft rough black", { 0.04f, 0.04f, 0.04f, 1.0f } },
};

// colors of the parts by their names when the material has no color: the material file is missing or the material
// has a texture image (its color is then the default grey of assimp)
const PartColor NAME_COLORS[] = {
    { "glass", { 0.05f, 0.07f, 0.09f, 1.0f } },
    { "tail light", { 0.75f, 0.04f, 0.03f, 1.0f } },
    { "emissive warm", { 0.75f, 0.04f, 0.03f, 1.0f } },
    { "light", { 0.95f, 0.95f, 0.90f, 1.0f } },
    { "red", { 0.70f, 0.05f, 0.05f, 1.0f } },
    { "tire", { 0.05f, 0.05f, 0.05f, 1.0f } },
    { "tyre", { 0.05f, 0.05f, 0.05f, 1.0f } },
    { "wheel", { 0.05f, 0.05f, 0.05f, 1.0f } },
    { "rim", { 0.55f, 0.56f, 0.58f, 1.0f } },
    { "chrome", { 0.70f, 0.71f, 0.73f, 1.0f } },
    { "steel", { 0.60f, 0.61f, 0.63f, 1.0f } },
    { "alumin", { 0.75f, 0.76f, 0.78f, 1.0f } },
    { "brass", { 0.71f, 0.60f, 0.35f, 1.0f } },
    { "beige", { 0.76f, 0.70f, 0.58f, 1.0f } },
    { "black", { 0.04f, 0.04f, 0.04f, 1.0f } },
    { "carbon", { 0.06f, 0.06f, 0.06f, 1.0f } },
    { "grille", { 0.03f, 0.03f, 0.03f, 1.0f } },
    { "trim", { 0.08f, 0.08f, 0.08f, 1.0f } },
};
const float DEFAULT_COLOR[4] = { 0.59f, 0.59f, 0.59f, 1.0f };
const float PAINT_COLOR[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

const ModelFile *findModel(ObjectShape shape)
{
    for (const ModelFile &model : MODEL_FILES) {
        if (model.shape == shape) {
            return &model;
        }
    }
    return nullptr;
}

std::string lower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

// the own format of the creator: .own_obj or .creator_obj (the same format)
bool isOwnObj(const std::string &filename)
{
    const std::string extension = lower(std::filesystem::path(filename).extension().string());
    return extension == ".own_obj" || extension == ".creator_obj";
}

// lowercase name without the number of a copy (".001" of blender)
std::string baseName(const std::string &name)
{
    std::string text = lower(name);
    const size_t dot = text.rfind('.');
    if (dot != std::string::npos && dot + 1 < text.size()
        && std::all_of(text.begin() + static_cast<long>(dot) + 1, text.end(), [](unsigned char c) { return std::isdigit(c); })) {
        text.erase(dot);
    }
    return text;
}

// true if the part or material name is one of the names (nullptr ends the list)
bool inList(const char *const *list, const std::string &partName, const std::string &materialName)
{
    for (int i=0;list[i];i++) {
        if (baseName(partName) == list[i] || baseName(materialName) == list[i]) {
            return true;
        }
    }
    return false;
}

// color in the list of the first lowercase part of the names, nullptr if none
const float *listColor(const PartColor *list, size_t count, const std::string &partName, const std::string &materialName)
{
    for (const std::string &name : { lower(partName), lower(materialName) }) {
        for (size_t i=0;i<count;i++) {
            if (name.find(list[i].name) != std::string::npos) {
                return list[i].color;
            }
        }
    }
    return nullptr;
}

// the cache file of the model: ~/.cache/prj-camera-image-creator/<model name>-<hash of the files>.mesh
std::string cacheFilename(const std::string &modelFile, const std::vector<std::string> &files)
{
    const char *cacheHome = getenv("XDG_CACHE_HOME");
    const char *home = getenv("HOME");
    std::filesystem::path directory;
    if (cacheHome && *cacheHome) {
        directory = cacheHome;
    } else if (home && *home) {
        directory = std::filesystem::path(home)/".cache";
    } else {
        return std::string();
    }
    // FNV-1a of the paths, sizes and change times
    uint64_t hash = 14695981039346656037ULL;
    auto add = [&hash](const std::string &text) {
        for (const unsigned char c : text) {
            hash = (hash ^ c)*1099511628211ULL;
        }
    };
    add(std::to_string(MODEL_CACHE_VERSION));
    for (const std::string &file : files) {
        std::error_code error;
        add(std::filesystem::absolute(file, error).string());
        const auto size = std::filesystem::file_size(file, error);
        add(error ? std::string("-") : std::to_string(size));
        const auto time = std::filesystem::last_write_time(file, error);
        add(error ? std::string("-") : std::to_string(time.time_since_epoch().count()));
    }
    char hex[17];
    snprintf(hex, sizeof(hex), "%016llx", static_cast<unsigned long long>(hash));
    return (directory/MODEL_CACHE_SUBDIRECTORY/(std::filesystem::path(modelFile).stem().string() + "-" + hex + ".mesh")).string();
}

} // namespace

bool ModelMesh::isModelShape(ObjectShape shape)
{
    return findModel(shape) != nullptr;
}

const char *ModelMesh::getObjectsFileKey(ObjectShape shape)
{
    const ModelFile *model = findModel(shape);
    return model ? model->key : "";
}

std::vector<ObjectShape> ModelMesh::getModelShapes()
{
    std::vector<ObjectShape> shapes;
    for (const ModelFile &model : MODEL_FILES) {
        shapes.push_back(model.shape);
    }
    return shapes;
}

const ModelMesh *ModelMesh::get(ObjectShape shape, const std::string &modelsDirectory)
{
    // loaded meshes of the shapes, nullptr if loading failed (it is not tried again)
    static std::map<ObjectShape, std::unique_ptr<ModelMesh>> loaded;
    const ModelFile *model = findModel(shape);
    if (!model) {
        return nullptr;
    }
    auto found = loaded.find(shape);
    if (found != loaded.end()) {
        return found->second.get();
    }
    if (modelsDirectory.empty()) {
        TG_ERROR_LOG(std::string("modelsDirectory of general.ini is missing, cannot load ") + model->modelFile);
        return nullptr;
    }
    const std::string modelFile = (std::filesystem::path(modelsDirectory)/model->modelFile).string();
    const std::string measureFile = (std::filesystem::path(modelsDirectory)/model->measureFile).string();
    std::filesystem::path materialFile = modelFile;
    materialFile.replace_extension(".mtl");
    // an .own_obj file is read fast, it has no cache file
    const std::string cacheFile = isOwnObj(modelFile) ? std::string()
                                                      : cacheFilename(modelFile, { modelFile, materialFile.string(), measureFile });
    std::unique_ptr<ModelMesh> mesh = std::make_unique<ModelMesh>();
    if (cacheFile.empty() || !mesh->readCache(cacheFile)) {
        mesh = std::make_unique<ModelMesh>();
        if (mesh->load(modelFile, measureFile, model->paintParts, model->glassParts)) {
            if (!cacheFile.empty()) {
                mesh->writeCache(cacheFile);
            }
        } else {
            mesh.reset();
        }
    }
    return loaded.emplace(shape, std::move(mesh)).first->second.get();
}

const VECTOR3 &ModelMesh::getSize() const
{
    return m_size;
}

bool ModelMesh::hasGlass() const
{
    return m_glassIndexCount > 0;
}

namespace {

// a part of a model (a mesh of one material) in the units and axes of the model file
struct ModelPart
{
    std::string name;
    std::string material;
    float diffuse[3] = { 0.0f, 0.0f, 0.0f };
    bool hasDiffuse = false;    // the material file has the color (Kd)
    float opacity = 1.0f;       // d of the material file
    bool hasTexture = false;    // the material has a texture image (map_Kd), its color is not used
    std::vector<float> positions;   // x/y/z of every vertex
    std::vector<float> normals;     // x/y/z of every vertex
    std::vector<float> st;          // s/t of every vertex
    std::vector<uint32_t> indices;  // 3 vertices of every triangle
};

#define OWN_OBJ_MAGIC       "OOBJ"
#define OWN_OBJ_VERSION     1

// true if the obj file has mtllib lines (before its vertices) and their files exist (relative to the obj file)
bool objMaterialFilesExist(const std::string &objFile)
{
    std::ifstream file(objFile);
    std::string line;
    bool found = false;
    while (std::getline(file, line)) {
        if (line.compare(0, 2, "v ") == 0 || line.compare(0, 2, "f ") == 0) {
            break;
        }
        if (line.compare(0, 7, "mtllib ") == 0) {
            std::string name = line.substr(7);
            name.erase(name.find_last_not_of(" \t\r") + 1);
            if (!std::filesystem::exists(std::filesystem::path(objFile).parent_path()/name)) {
                TG_ERROR_LOG(objFile + ": material file", name, "is missing");
                return false;
            }
            found = true;
        }
    }
    return found;
}

bool readAssimpParts(const std::string &modelFile, std::vector<ModelPart> &parts)
{
    TG_DEBUG_LOG("Loading", modelFile, "(a large model takes long the first time)");
    Assimp::Importer importer;
    // the nodes are walked here with their transformations (aiProcess_PreTransformVertices would join the meshes of
    // the same material and lose their names, for example "glass window" and "glass lamp" of the R8)
    const aiScene *scene = importer.ReadFile(modelFile, aiProcess_Triangulate | aiProcess_GenSmoothNormals
                                             | aiProcess_JoinIdenticalVertices);
    if (!scene || !scene->mRootNode || scene->mNumMeshes == 0) {
        TG_ERROR_LOG("Cannot load", modelFile + ":", importer.GetErrorString());
        return false;
    }
    // every mesh of every node with the transformation of the node (a mesh can be in many nodes)
    std::vector<std::pair<unsigned int, aiMatrix4x4>> instances;
    std::vector<std::pair<const aiNode *, aiMatrix4x4>> stack = { { scene->mRootNode, scene->mRootNode->mTransformation } };
    while (!stack.empty()) {
        const auto [node, transform] = stack.back();
        stack.pop_back();
        for (unsigned int i=0;i<node->mNumMeshes;i++) {
            if (node->mMeshes[i] < scene->mNumMeshes) {
                instances.emplace_back(node->mMeshes[i], transform);
            }
        }
        for (unsigned int i=node->mNumChildren;i>0;i--) {
            stack.emplace_back(node->mChildren[i - 1], transform*node->mChildren[i - 1]->mTransformation);
        }
    }
    // an obj file without its material files (mtllib lines) gets default materials of the names, their colors are
    // not used
    bool hasMaterialFile = true;
    if (lower(std::filesystem::path(modelFile).extension().string()) == ".obj") {
        hasMaterialFile = objMaterialFilesExist(modelFile);
    }
    for (const auto &[meshIndex, transform] : instances) {
        const aiMesh *mesh = scene->mMeshes[meshIndex];
        const aiMaterial *material = mesh->mMaterialIndex < scene->mNumMaterials ? scene->mMaterials[mesh->mMaterialIndex]
                                                                                  : nullptr;
        // the normals turn with the inverse transpose of the transformation
        aiMatrix3x3 normalTransform(transform);
        normalTransform.Inverse().Transpose();
        ModelPart part;
        part.name = mesh->mName.C_Str();
        if (material) {
            aiString materialName;
            aiColor4D diffuse;
            material->Get(AI_MATKEY_NAME, materialName);
            part.material = materialName.C_Str();
            if (hasMaterialFile) {
                material->Get(AI_MATKEY_OPACITY, part.opacity);
                if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS) {
                    part.diffuse[0] = diffuse.r;
                    part.diffuse[1] = diffuse.g;
                    part.diffuse[2] = diffuse.b;
                    part.hasDiffuse = true;
                }
                part.hasTexture = material->GetTextureCount(aiTextureType_DIFFUSE) > 0;
            }
        }
        for (unsigned int i=0;i<mesh->mNumVertices;i++) {
            const aiVector3D p = transform*mesh->mVertices[i];
            const aiVector3D n = (normalTransform*(mesh->HasNormals() ? mesh->mNormals[i] : aiVector3D(0.0f, 1.0f, 0.0f))).Normalize();
            const aiVector3D st = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D();
            part.positions.insert(part.positions.end(), { p.x, p.y, p.z });
            part.normals.insert(part.normals.end(), { n.x, n.y, n.z });
            part.st.insert(part.st.end(), { st.x, st.y });
        }
        for (unsigned int f=0;f<mesh->mNumFaces;f++) {
            const aiFace &face = mesh->mFaces[f];
            // points and lines of the model are not drawn
            if (face.mNumIndices == 3) {
                part.indices.insert(part.indices.end(), { face.mIndices[0], face.mIndices[1], face.mIndices[2] });
            }
        }
        parts.push_back(std::move(part));
    }
    return true;
}

/*
 * .own_obj file (little-endian): char[4] OWN_OBJ_MAGIC, uint32_t OWN_OBJ_VERSION, uint32_t count of the parts, then
 * every part: uint32_t length and the characters of its name, the same of its material name, float diffuse r/g/b,
 * uint8_t has diffuse, float opacity, uint8_t has texture, uint32_t vertex count, uint32_t triangle count, float
 * x/y/z of the vertices, float normal x/y/z of the vertices, float s/t of the vertices, uint32_t 3 vertices of the
 * triangles. The coordinates are those of the model file (its units and axes).
 */
bool readOwnObj(const std::string &filename, std::vector<ModelPart> &parts)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        TG_ERROR_LOG("Cannot open", filename);
        return false;
    }
    auto readUint32 = [&file]() {
        uint32_t value = 0;
        file.read(reinterpret_cast<char *>(&value), sizeof(value));
        return value;
    };
    auto readString = [&file, &readUint32]() {
        std::string text(readUint32(), '\0');
        file.read(text.data(), static_cast<std::streamsize>(text.size()));
        return text;
    };
    auto readFloats = [&file](std::vector<float> &values, size_t count) {
        values.resize(count);
        file.read(reinterpret_cast<char *>(values.data()), static_cast<std::streamsize>(count*sizeof(float)));
    };
    char magic[4] = { 0, 0, 0, 0 };
    file.read(magic, 4);
    if (memcmp(magic, OWN_OBJ_MAGIC, 4) != 0 || readUint32() != OWN_OBJ_VERSION) {
        TG_ERROR_LOG(filename, "is not an .own_obj file of version", OWN_OBJ_VERSION);
        return false;
    }
    const uint32_t count = readUint32();
    for (uint32_t i=0;i<count && file;i++) {
        ModelPart part;
        uint8_t flag = 0;
        part.name = readString();
        part.material = readString();
        file.read(reinterpret_cast<char *>(part.diffuse), sizeof(part.diffuse));
        file.read(reinterpret_cast<char *>(&flag), 1);
        part.hasDiffuse = flag != 0;
        file.read(reinterpret_cast<char *>(&part.opacity), sizeof(part.opacity));
        file.read(reinterpret_cast<char *>(&flag), 1);
        part.hasTexture = flag != 0;
        const uint32_t vertexCount = readUint32();
        const uint32_t triangleCount = readUint32();
        if (!file || vertexCount > 100000000u || triangleCount > 100000000u) {
            break;
        }
        readFloats(part.positions, static_cast<size_t>(vertexCount)*3);
        readFloats(part.normals, static_cast<size_t>(vertexCount)*3);
        readFloats(part.st, static_cast<size_t>(vertexCount)*2);
        part.indices.resize(static_cast<size_t>(triangleCount)*3);
        file.read(reinterpret_cast<char *>(part.indices.data()), static_cast<std::streamsize>(part.indices.size()*sizeof(uint32_t)));
        if (std::any_of(part.indices.begin(), part.indices.end(), [vertexCount](uint32_t v) { return v >= vertexCount; })) {
            TG_ERROR_LOG(filename + ": part", part.name, "has invalid triangles");
            return false;
        }
        parts.push_back(std::move(part));
    }
    if (!file || parts.size() != count) {
        TG_ERROR_LOG(filename, "is not complete");
        return false;
    }
    return true;
}

bool writeOwnObj(const std::string &filename, const std::vector<ModelPart> &parts)
{
    const std::string temporary = filename + ".tmp" + std::to_string(getpid());
    {
        std::ofstream file(temporary, std::ios::binary);
        auto writeUint32 = [&file](uint32_t value) {
            file.write(reinterpret_cast<const char *>(&value), sizeof(value));
        };
        auto writeString = [&file, &writeUint32](const std::string &text) {
            writeUint32(static_cast<uint32_t>(text.size()));
            file.write(text.data(), static_cast<std::streamsize>(text.size()));
        };
        auto writeFloats = [&file](const std::vector<float> &values) {
            file.write(reinterpret_cast<const char *>(values.data()), static_cast<std::streamsize>(values.size()*sizeof(float)));
        };
        file.write(OWN_OBJ_MAGIC, 4);
        writeUint32(OWN_OBJ_VERSION);
        writeUint32(static_cast<uint32_t>(parts.size()));
        for (const ModelPart &part : parts) {
            const uint8_t hasDiffuse = part.hasDiffuse ? 1 : 0;
            const uint8_t hasTexture = part.hasTexture ? 1 : 0;
            writeString(part.name);
            writeString(part.material);
            file.write(reinterpret_cast<const char *>(part.diffuse), sizeof(part.diffuse));
            file.write(reinterpret_cast<const char *>(&hasDiffuse), 1);
            file.write(reinterpret_cast<const char *>(&part.opacity), sizeof(part.opacity));
            file.write(reinterpret_cast<const char *>(&hasTexture), 1);
            writeUint32(static_cast<uint32_t>(part.positions.size()/3));
            writeUint32(static_cast<uint32_t>(part.indices.size()/3));
            writeFloats(part.positions);
            writeFloats(part.normals);
            writeFloats(part.st);
            file.write(reinterpret_cast<const char *>(part.indices.data()), static_cast<std::streamsize>(part.indices.size()*sizeof(uint32_t)));
        }
        if (!file) {
            TG_ERROR_LOG("Cannot write", temporary);
            return false;
        }
    }
    std::error_code error;
    std::filesystem::rename(temporary, filename, error);
    if (error) {
        TG_ERROR_LOG("Cannot write", filename);
        std::filesystem::remove(temporary, error);
        return false;
    }
    return true;
}

} // namespace

/*!
 * \brief ModelMesh::convert
 * reads a model file (obj... with assimp, the materials of its material file) and writes it as an .own_obj file
 * \return 0 on success
 */
int ModelMesh::convert(const std::string &modelFile, const std::string &ownObjFile)
{
    std::vector<ModelPart> parts;
    if (!readAssimpParts(modelFile, parts) || !writeOwnObj(ownObjFile, parts)) {
        return 1;
    }
    size_t triangles = 0, vertices = 0;
    for (const ModelPart &part : parts) {
        triangles += part.indices.size()/3;
        vertices += part.positions.size()/3;
    }
    TG_DEBUG_LOG("Saved", ownObjFile + ":", parts.size(), "parts,", vertices, "vertices,", triangles, "triangles");
    return 0;
}

/*!
 * \brief ModelMesh::load
 * \param modelFile 3D model file: .own_obj or a file that assimp reads (obj...)
 * \param measureFile Length=<mm> of the real car
 * \param paintParts names of the paint parts, nullptr ends the list
 * \param glassParts names of the glass parts (in addition to the transparent materials), nullptr ends the list
 * \return true on success
 */
bool ModelMesh::load(const std::string &modelFile, const std::string &measureFile, const char *const *paintParts,
                     const char *const *glassParts)
{
    IniFile measure;
    float length = 0.0f;
    if (!measure.read(measureFile) || !measure.getFloat("Length", &length) || length <= 0.0f) {
        TG_ERROR_LOG(measureFile + ": Length (mm) is missing or not positive");
        return false;
    }
    std::vector<ModelPart> parts;
    if (isOwnObj(modelFile) ? !readOwnObj(modelFile, parts) : !readAssimpParts(modelFile, parts)) {
        return false;
    }
    // the box of the model in its own units
    float low[3] = { 1e30f, 1e30f, 1e30f }, high[3] = { -1e30f, -1e30f, -1e30f };
    for (const ModelPart &part : parts) {
        for (size_t i=0;i<part.positions.size();i++) {
            low[i%3] = std::min(low[i%3], part.positions[i]);
            high[i%3] = std::max(high[i%3], part.positions[i]);
        }
    }
    if (high[2] <= low[2]) {
        TG_ERROR_LOG(modelFile + ": the model has no length");
        return false;
    }
    const float center[3] = { (low[0] + high[0])/2.0f, (low[1] + high[1])/2.0f, (low[2] + high[2])/2.0f };
    // model units to opengl units, the length of the model becomes the Length of the measure file
    const float scale = length/(high[2] - low[2])/MM_TO_OPENGL_UNIT;
    m_size = { (high[0] - low[0])*scale, (high[1] - low[1])*scale, (high[2] - low[2])*scale };
    // vertices and triangles of the paint (0), the details (1) and the glass (2)
    std::vector<MeshVertex> vertices[3];
    std::vector<uint32_t> indices[3];
    for (const ModelPart &part : parts) {
        const bool isPaint = inList(paintParts, part.name, part.material);
        const bool transparent = part.opacity < 1.0f;
        const bool isGlass = !isPaint && (transparent || inList(glassParts, part.name, part.material));
        float materialColor[4] = { part.diffuse[0], part.diffuse[1], part.diffuse[2], 1.0f };
        const float *color = PAINT_COLOR;
        if (isGlass) {
            if (transparent && part.hasDiffuse) {
                materialColor[3] = part.opacity;
                color = materialColor;
            } else {
                color = GLASS_COLOR;
            }
        } else if (!isPaint) {
            color = listColor(OWN_COLORS, std::size(OWN_COLORS), part.name, part.material);
            if (!color && part.hasDiffuse && !part.hasTexture) {
                color = materialColor;
            }
            if (!color) {
                color = listColor(NAME_COLORS, std::size(NAME_COLORS), part.name, part.material);
            }
            if (!color) {
                color = DEFAULT_COLOR;
            }
        }
        const int list = isPaint ? 0 : (isGlass ? 2 : 1);
        const uint32_t first = static_cast<uint32_t>(vertices[list].size());
        for (size_t i=0;i<part.positions.size()/3;i++) {
            const float *p = &part.positions[i*3];
            const float *n = &part.normals[i*3];
            // x is mirrored: right-handed model to the left-handed scene
            vertices[list].push_back( { { -(p[0] - center[0])*scale, (p[1] - center[1])*scale, (p[2] - center[2])*scale },
                                        { -n[0], n[1], n[2] }, { part.st[i*2], part.st[i*2 + 1] },
                                        { color[0], color[1], color[2], color[3] } } );
        }
        for (const uint32_t index : part.indices) {
            indices[list].push_back(first + index);
        }
    }
    for (int list=0;list<3;list++) {
        const uint32_t offset = static_cast<uint32_t>(m_vertices.size());
        m_vertices.insert(m_vertices.end(), vertices[list].begin(), vertices[list].end());
        for (const uint32_t index : indices[list]) {
            m_indices.push_back(offset + index);
        }
    }
    m_paintIndexCount = static_cast<uint32_t>(indices[0].size());
    m_detailIndexCount = static_cast<uint32_t>(indices[1].size());
    m_glassIndexCount = static_cast<uint32_t>(indices[2].size());
    if (m_indices.empty()) {
        TG_ERROR_LOG(modelFile + ": the model has no triangles");
        return false;
    }
    TG_DEBUG_LOG("Loaded", modelFile + ",", m_indices.size()/3, "triangles, size", m_size.x*MM_TO_OPENGL_UNIT, "x",
                 m_size.y*MM_TO_OPENGL_UNIT, "x", m_size.z*MM_TO_OPENGL_UNIT, "mm");
    return true;
}

/*!
 * \brief ModelMesh::readCache
 * cache file: char[4] MODEL_CACHE_MAGIC, uint32_t MODEL_CACHE_VERSION, float size x/y/z, uint32_t vertex count,
 * uint32_t paint, detail and glass index counts, the vertices (MeshVertex) and the indices (uint32_t)
 * \return true if the file exists and is complete
 */
bool ModelMesh::readCache(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    char magic[4];
    uint32_t version = 0, vertexCount = 0;
    file.read(magic, 4);
    file.read(reinterpret_cast<char *>(&version), sizeof(version));
    file.read(reinterpret_cast<char *>(&m_size), sizeof(m_size));
    file.read(reinterpret_cast<char *>(&vertexCount), sizeof(vertexCount));
    file.read(reinterpret_cast<char *>(&m_paintIndexCount), sizeof(m_paintIndexCount));
    file.read(reinterpret_cast<char *>(&m_detailIndexCount), sizeof(m_detailIndexCount));
    file.read(reinterpret_cast<char *>(&m_glassIndexCount), sizeof(m_glassIndexCount));
    if (!file || memcmp(magic, MODEL_CACHE_MAGIC, 4) != 0 || version != MODEL_CACHE_VERSION) {
        return false;
    }
    m_vertices.resize(vertexCount);
    m_indices.resize(static_cast<size_t>(m_paintIndexCount) + m_detailIndexCount + m_glassIndexCount);
    file.read(reinterpret_cast<char *>(m_vertices.data()), static_cast<std::streamsize>(m_vertices.size()*sizeof(MeshVertex)));
    file.read(reinterpret_cast<char *>(m_indices.data()), static_cast<std::streamsize>(m_indices.size()*sizeof(uint32_t)));
    if (!file || file.peek() != std::ifstream::traits_type::eof()) {
        TG_ERROR_LOG(filename, "is not complete, the model is loaded again");
        return false;
    }
    for (const uint32_t index : m_indices) {
        if (index >= vertexCount) {
            TG_ERROR_LOG(filename, "is invalid, the model is loaded again");
            return false;
        }
    }
    TG_DEBUG_LOG("Loaded", filename + ",", m_indices.size()/3, "triangles");
    return true;
}

/*!
 * \brief ModelMesh::writeCache
 * writes the cache file (a temporary file renamed at the end, so a crash leaves no half file, processes that load
 * the same model at the same time write the same file) and removes the older cache files of the same model
 */
void ModelMesh::writeCache(const std::string &filename) const
{
    const std::filesystem::path path(filename);
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    const std::string temporary = filename + ".tmp" + std::to_string(getpid());
    {
        std::ofstream file(temporary, std::ios::binary);
        const uint32_t version = MODEL_CACHE_VERSION;
        const uint32_t vertexCount = static_cast<uint32_t>(m_vertices.size());
        file.write(MODEL_CACHE_MAGIC, 4);
        file.write(reinterpret_cast<const char *>(&version), sizeof(version));
        file.write(reinterpret_cast<const char *>(&m_size), sizeof(m_size));
        file.write(reinterpret_cast<const char *>(&vertexCount), sizeof(vertexCount));
        file.write(reinterpret_cast<const char *>(&m_paintIndexCount), sizeof(m_paintIndexCount));
        file.write(reinterpret_cast<const char *>(&m_detailIndexCount), sizeof(m_detailIndexCount));
        file.write(reinterpret_cast<const char *>(&m_glassIndexCount), sizeof(m_glassIndexCount));
        file.write(reinterpret_cast<const char *>(m_vertices.data()), static_cast<std::streamsize>(m_vertices.size()*sizeof(MeshVertex)));
        file.write(reinterpret_cast<const char *>(m_indices.data()), static_cast<std::streamsize>(m_indices.size()*sizeof(uint32_t)));
        if (!file) {
            TG_ERROR_LOG("Cannot write", temporary);
            file.close();
            std::filesystem::remove(temporary, error);
            return;
        }
    }
    std::filesystem::rename(temporary, filename, error);
    if (error) {
        TG_ERROR_LOG("Cannot write", filename);
        std::filesystem::remove(temporary, error);
        return;
    }
    // older cache files of the same model: <model name>-<16 hex digits>.mesh
    const std::string stem = path.stem().string();
    const std::string modelName = stem.substr(0, stem.size() - 17);
    for (const auto &entry : std::filesystem::directory_iterator(path.parent_path(), error)) {
        const std::string name = entry.path().filename().string();
        if (entry.path() != path && name.size() == modelName.size() + 22 && name.compare(0, modelName.size() + 1, modelName + "-") == 0
            && entry.path().extension() == ".mesh") {
            std::filesystem::remove(entry.path(), error);
        }
    }
    TG_DEBUG_LOG("Saved", filename);
}

int ModelMesh::upload() const
{
    if (m_vertexArrayObject) {
        return 0;
    }
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glGenBuffers(1, &m_indexBufferObject);
    glBindVertexArray(m_vertexArrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vertices.size()*sizeof(MeshVertex)), m_vertices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferObject);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_indices.size()*sizeof(uint32_t)), m_indices.data(),
                 GL_STATIC_DRAW);
    const GLsizei stride = sizeof(MeshVertex);
    const GLuint *attributes = CameraShader::m_shaderAttributeIndex;
    glVertexAttribPointer(attributes[ShaderAttributes3d::AttribPosition], 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(MeshVertex, position)));
    glEnableVertexAttribArray(attributes[ShaderAttributes3d::AttribPosition]);
    glVertexAttribPointer(attributes[ShaderAttributes3d::AttribNormal], 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(MeshVertex, normal)));
    glEnableVertexAttribArray(attributes[ShaderAttributes3d::AttribNormal]);
    glVertexAttribPointer(attributes[ShaderAttributes3d::AttribTextCoord], 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(MeshVertex, st)));
    glEnableVertexAttribArray(attributes[ShaderAttributes3d::AttribTextCoord]);
    glVertexAttribPointer(attributes[ShaderAttributes3d::AttribColor], 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(offsetof(MeshVertex, color)));
    glEnableVertexAttribArray(attributes[ShaderAttributes3d::AttribColor]);
    glBindVertexArray(0);
    // the buffers have the mesh, the memory is not needed anymore
    const_cast<ModelMesh *>(this)->m_vertices = std::vector<MeshVertex>();
    return glGetError() == GL_NO_ERROR ? 0 : 1;
}

void ModelMesh::drawRange(uint32_t first, uint32_t count) const
{
    if (count) {
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(count), GL_UNSIGNED_INT,
                       reinterpret_cast<void *>(static_cast<size_t>(first)*sizeof(uint32_t)));
    }
}

void ModelMesh::drawOpaque(bool opaqueGlass, GLuint paintTexture, GLuint detailTexture) const
{
    glBindVertexArray(m_vertexArrayObject);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, paintTexture);
    drawRange(0, m_paintIndexCount);
    glBindTexture(GL_TEXTURE_2D, detailTexture);
    drawRange(m_paintIndexCount, m_detailIndexCount + (opaqueGlass ? m_glassIndexCount : 0));
    glBindVertexArray(0);
}

void ModelMesh::drawGlass(GLuint detailTexture) const
{
    glBindVertexArray(m_vertexArrayObject);
    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, detailTexture);
    drawRange(m_paintIndexCount + m_detailIndexCount, m_glassIndexCount);
    glBindVertexArray(0);
}
