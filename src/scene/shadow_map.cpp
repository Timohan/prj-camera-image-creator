#include "shadow_map.h"
#include <GL/gl.h>
#include <algorithm>
#include <iostream>

// size of the shadow map tile of one spot light in pixels
#define SHADOW_MAP_SIZE             2048
// tiles in one row of the shadow map texture
#define SHADOW_MAP_COLUMNS          4
#define SHADOW_NEAR_PLANE           0.5f
#define SHADOW_FAR_PLANE            10000.0f
// shadow map sees this many degrees more than the outer angle of the spot light
#define SHADOW_ANGLE_MARGIN         2.0f
// widest field of view of the shadow map in degrees, light outside of it has no shadows
#define SHADOW_MAX_FIELD_OF_VIEW    160.0f
// polygon offset of the depth of the shadow casters
#define SHADOW_POLYGON_OFFSET_FACTOR    2.0f
#define SHADOW_POLYGON_OFFSET_UNITS     4.0f
// texture unit of the shadow map, texture unit 0 is for the textures of the objects
#define SHADOW_TEXTURE_UNIT         1

ShadowMap::ShadowMap() :
    m_texture(0),
    m_framebuffer(0),
    m_texelSize{ 0.0f, 0.0f }
{
}

ShadowMap::~ShadowMap()
{
    if (m_framebuffer) {
        glDeleteFramebuffers(1, &m_framebuffer);
    }
    if (m_texture) {
        glDeleteTextures(1, &m_texture);
    }
}

/*!
 * \brief ShadowMap::create
 * renders the shadow map of every spot light, the shadow casters and the lights must not move after this
 * \param shader shader program in use
 * \param listSpotLight spot lights
 * \param render renders the shadow casters
 * \return 0 on success
 */
int ShadowMap::create(GLuint shader, const std::vector<SpotLightConfig> &listSpotLight, const RenderFunction &render)
{
    const int count = static_cast<int>(listSpotLight.size());
    const int columns = std::max(1, std::min(count, SHADOW_MAP_COLUMNS));
    const int rows = std::max(1, (count + SHADOW_MAP_COLUMNS - 1)/SHADOW_MAP_COLUMNS);
    GLint maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    const int tileSize = std::min(SHADOW_MAP_SIZE, static_cast<int>(maxTextureSize)/columns);
    const int width = columns*tileSize;
    const int height = rows*tileSize;
    m_texelSize[0] = 1.0f/static_cast<GLfloat>(width);
    m_texelSize[1] = 1.0f/static_cast<GLfloat>(height);

    // samplers of different types must not use the same texture unit
    unbind();
    glUniform1i(glGetUniformLocation(shader, "shadowMap"), SHADOW_TEXTURE_UNIT);

    // shader compares the depth of the pixel with the texture (sampler2DShadow), linear filter
    // compares 4 texels, the texture is created also without lights, because the shader has the sampler
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLint previousFramebuffer, previousViewport[4];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Shadow map framebuffer is not complete" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
        return 1;
    }
    glViewport(0, 0, width, height);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(SHADOW_POLYGON_OFFSET_FACTOR, SHADOW_POLYGON_OFFSET_UNITS);

    m_listLightShadow.clear();
    for (int i=0;i<count;i++) {
        const SpotLightConfig &light = listSpotLight[i];
        const float fieldOfView = std::min(2.0f*(light.outerAngle + SHADOW_ANGLE_MARGIN), SHADOW_MAX_FIELD_OF_VIEW)*PI_VALUE/180.0f;
        const VECTOR3 direction = Math3D::Vec3Normalize(light.direction);
        VECTOR3 at, up = { 0.0f, 1.0f, 0.0f };
        // light pointing straight up or down needs another up vector
        if (fabsf(direction.y) > 0.99f) {
            up = { 0.0f, 0.0f, 1.0f };
        }
        at.x = light.position.x + direction.x;
        at.y = light.position.y + direction.y;
        at.z = light.position.z + direction.z;

        LightShadow shadow;
        Math3D::MatrixPerspectiveFovLH(&shadow.projection, fieldOfView, 1.0f, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
        Math3D::MatrixLookAtLH(&shadow.view, light.position, at, up);
        const int column = i%SHADOW_MAP_COLUMNS;
        const int row = i/SHADOW_MAP_COLUMNS;
        shadow.tile[0] = static_cast<GLfloat>(column*tileSize)/static_cast<GLfloat>(width);
        shadow.tile[1] = static_cast<GLfloat>(row*tileSize)/static_cast<GLfloat>(height);
        shadow.tile[2] = static_cast<GLfloat>(tileSize)/static_cast<GLfloat>(width);
        shadow.tile[3] = static_cast<GLfloat>(tileSize)/static_cast<GLfloat>(height);
        shadow.texelRate = 2.0f*tanf(fieldOfView/2.0f)/static_cast<float>(tileSize);
        m_listLightShadow.push_back(shadow);

        glViewport(column*tileSize, row*tileSize, tileSize, tileSize);
        render(shadow.projection, shadow.view);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    return 0;
}

/*!
 * \brief ShadowMap::setupShader
 * binds the shadow map texture and sets the shadow uniforms of the spot lights,
 * spot lights are in the same order as in create()
 * \param shader shader program in use
 */
void ShadowMap::setupShader(GLuint shader) const
{
    const GLsizei count = static_cast<GLsizei>(m_listLightShadow.size());
    std::vector<GLfloat> projections, views, tiles, texelRates;

    for (const LightShadow &shadow : m_listLightShadow) {
        projections.insert(projections.end(), shadow.projection.m, shadow.projection.m + 16);
        views.insert(views.end(), shadow.view.m, shadow.view.m + 16);
        tiles.insert(tiles.end(), shadow.tile, shadow.tile + 4);
        texelRates.push_back(shadow.texelRate);
    }
    glActiveTexture(GL_TEXTURE0 + SHADOW_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(shader, "shadowMap"), SHADOW_TEXTURE_UNIT);
    glUniform2fv(glGetUniformLocation(shader, "shadowMapTexelSize"), 1, m_texelSize);
    if (count > 0) {
        glUniformMatrix4fv(glGetUniformLocation(shader, "lightProjection"), count, GL_FALSE, projections.data());
        glUniformMatrix4fv(glGetUniformLocation(shader, "lightView"), count, GL_FALSE, views.data());
        glUniform4fv(glGetUniformLocation(shader, "lightShadowTile"), count, tiles.data());
        glUniform1fv(glGetUniformLocation(shader, "lightShadowTexelRate"), count, texelRates.data());
    }
}

/*!
 * \brief ShadowMap::unbind
 * unbinds the shadow map texture, so it is not read while it is rendered
 */
void ShadowMap::unbind() const
{
    glActiveTexture(GL_TEXTURE0 + SHADOW_TEXTURE_UNIT);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
}
