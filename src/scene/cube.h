#ifndef CUBE_H
#define CUBE_H

#include <GL/glew.h>
#include <GL/gl.h>
#include "objects_config.h"

class ModelMesh;

class Cube
{
public:
    explicit Cube();
    ~Cube();

    void setup(const CubeConfig &config);
    int init();
    // the opaque triangles, in the distance passes (opaqueGlass) also the glass as solid
    void render(bool opaqueGlass = false);
    // the transparent glass (the windows of the model cars), after all opaque triangles of the scene
    void renderGlass();
    bool hasGlass() const;
    const CubeConfig &getConfig() const;

private:
    CubeConfig m_config;
    GLuint m_vertexArrayObject;
    GLuint m_vertexBufferObject;
    GLuint m_textureIndex;
    // cars and people: white texture of their details (windows, tires, skin...), they are after the paint vertices
    GLuint m_detailTexture;
    int m_paintVerticesCount;
    int m_verticesCount;
    // the glass vertices (transparent) are the last m_glassVerticesCount of the detail vertices
    int m_glassVerticesCount;
    // the model cars draw the vertex buffer of their model (no own vertices)
    const ModelMesh *m_model;

    int initTextures();
    void setModelTransform(bool on);
};

#endif // CUBE_H
