#ifndef SHADOW_MAP_H
#define SHADOW_MAP_H

#include <functional>
#include <vector>
#include <GL/glew.h>
#include "../math/math3d.h"
#include "objects_config.h"

/*!
 * \brief ShadowMap
 * Shadow maps of the spot lights. Depth of the shadow casters is rendered from every spot light
 * into its own tile of one depth texture, the shader compares the depth of the pixel with it.
 */
class ShadowMap
{
public:
    // renders the shadow casters with the projection and view matrices
    typedef std::function<void(const Matrix4x4 &projection, const Matrix4x4 &view)> RenderFunction;

    ShadowMap();
    ~ShadowMap();
    ShadowMap(const ShadowMap &) = delete;
    ShadowMap &operator=(const ShadowMap &) = delete;

    int create(GLuint shader, const std::vector<SpotLightConfig> &listSpotLight, const RenderFunction &render);
    void setupShader(GLuint shader) const;

private:
    struct LightShadow
    {
        Matrix4x4 projection;
        Matrix4x4 view;
        GLfloat tile[4];        // offset x/y and scale x/y of the tile in the texture
        GLfloat texelRate;      // size of one texel at distance 1.0
    };
    GLuint m_texture;
    GLuint m_framebuffer;
    GLfloat m_texelSize[2];     // size of one texel in texture coordinates
    std::vector<LightShadow> m_listLightShadow;

    void unbind() const;
};

#endif // SHADOW_MAP_H
