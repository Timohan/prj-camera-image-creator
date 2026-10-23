#include "triangle.h"
#include "../shader/camera_shader.h"

struct TriangleVertice
{
    float x, y, z;
    float r, g, b, a;
};

Triangle::Triangle() :
    m_vertexArrayObject(0),
    m_vertexBufferObject(0),
    m_verticesCount(0)
{
}

Triangle::~Triangle()
{
    if (m_vertexArrayObject) {
        glDeleteVertexArrays(1, &m_vertexArrayObject);
    }
    if (m_vertexBufferObject) {
        glDeleteBuffers(1, &m_vertexBufferObject);
    }
}

int Triangle::init()
{
    const TriangleVertice listVertices[] = {
        {  0.0f, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f },
        { -1.0f, 0.0f, 1.0f,   0.0f, 1.0f, 0.0f, 1.0f },
        {  1.0f, 0.0f, 1.0f,   0.0f, 0.0f, 1.0f, 1.0f },
    };

    m_verticesCount = sizeof(listVertices)/sizeof(listVertices[0]);
    glGenVertexArrays(1, &m_vertexArrayObject);
    glGenBuffers(1, &m_vertexBufferObject);
    glBindVertexArray(m_vertexArrayObject);

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferObject);
    glBufferData(GL_ARRAY_BUFFER, sizeof(listVertices), listVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition], 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glVertexAttribPointer(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor], 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glBindVertexArray(0);
    return 0;
}

void Triangle::render()
{
    glBindVertexArray(m_vertexArrayObject);
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribPosition]);
    glEnableVertexAttribArray(CameraShader::m_shaderAttributeIndex[ShaderAttributes3d::AttribColor]);
    glDrawArrays(GL_TRIANGLES, 0, m_verticesCount);
    glBindVertexArray(0);
}
