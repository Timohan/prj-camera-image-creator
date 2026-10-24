#ifndef CAMERA_IMAGE_SAVER_H
#define CAMERA_IMAGE_SAVER_H

#include <functional>
#include <string>
#include <opencv2/core.hpp>
#include "cameraobject.h"

/*!
 * \brief CameraImageSaver
 * Saves the image of a camera object as png. The scene is rendered offscreen with
 * the projection of the camera (resolution, focal length, sensor size,
 * principal point, roll), lens distortion is simulated with OpenCV pinhole model
 * (radial k1-k6, tangential p1-p2) or OpenCV fisheye model (k1-k4), then
 * depth of field (aperture, focus distance) is simulated.
 */
class CameraImageSaver
{
public:
    // renders the scene with the projection and view matrices to the current framebuffer
    typedef std::function<void(const Matrix4x4 &projection, const Matrix4x4 &view)> RenderFunction;

    static bool save(const CameraObject &camera, const RenderFunction &render,
                     float zNear, float zFar, const std::string &filename);

private:
    static bool createPinholeImage(const CameraObject &camera, const RenderFunction &render,
                                   float zNear, float zFar, cv::Mat *image);
    static bool createFisheyeImage(const CameraObject &camera, const RenderFunction &render,
                                   float zNear, float zFar, cv::Mat *image);
    static bool renderImage(const RenderFunction &render, const Matrix4x4 &projection, const Matrix4x4 &view,
                            float zNear, float zFar, int width, int height, cv::Mat *image, cv::Mat *depth);
    static cv::Vec3b sampleLinear(const cv::Mat &image, float column, float row);
    static void applyDepthOfField(const CameraObjectConfig &config, const cv::Mat &depth, cv::Mat *image);
    static cv::Rect getDistortedArea(const cv::Mat &cameraMatrix, const cv::Mat &distortion, int width, int height);
};

#endif // CAMERA_IMAGE_SAVER_H
