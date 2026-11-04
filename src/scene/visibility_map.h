#ifndef VISIBILITY_MAP_H
#define VISIBILITY_MAP_H

#include <functional>
#include <vector>
#include <GL/glew.h>
#include "../math/math3d.h"
#include "cameraobjects_config.h"

/*!
 * \brief VisibilityMap
 * Shows which surfaces the camera objects can see. Distance from every camera object to the nearest surface is
 * rendered to the 6 faces of a cube around it (one float texture of 6 faces side by side), the shader blacks out
 * the pixels that are outside of the image of every camera (lens of the saved image) or behind a nearer surface
 * in every camera. Sky is seen if a camera sees it without a surface in front of it.
 */
class VisibilityMap
{
public:
    // renders the distance from the camera position to every pixel with the projection and view matrices,
    // cameraIndex is the camera object that is not rendered (its own cone)
    typedef std::function<void(const Matrix4x4 &projection, const Matrix4x4 &view, const VECTOR3 &cameraPosition,
                               int cameraIndex)> RenderFunction;

    VisibilityMap();
    ~VisibilityMap();
    VisibilityMap(const VisibilityMap &) = delete;
    VisibilityMap &operator=(const VisibilityMap &) = delete;

    int create(const std::vector<CameraObjectConfig> &listCamera, const RenderFunction &render);
    void setupShader(GLuint shader, bool enabled) const;
    void unbind() const;

private:
    struct CameraVisibility
    {
        VECTOR3 position;
        VECTOR3 right;
        VECTOR3 up;
        VECTOR3 forward;
        GLfloat lens[4];        // fx, fy, cx, cy in pixels of the saved image
        GLfloat size[4];        // width, height, 1 for the fisheye model, largest (x/z)^2 + (y/z)^2 or fisheye angle
        GLfloat distortion[8];  // pinhole k1, k2, p1, p2, k3, k4, k5, k6 or fisheye k1-k4
        GLfloat area[4];        // pinhole: rendered undistorted area left, top, right, bottom in pixels
        GLuint texture;
    };
    std::vector<CameraVisibility> m_listCamera;
    int m_faceSize;

    void clear();
};

#endif // VISIBILITY_MAP_H
