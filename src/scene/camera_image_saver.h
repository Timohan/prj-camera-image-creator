#ifndef CAMERA_IMAGE_SAVER_H
#define CAMERA_IMAGE_SAVER_H

#include <functional>
#include <string>
#include <opencv2/core.hpp>
#include "cameraobject.h"

/*!
 * \brief CameraImageSaver
 * Saves the image of a camera object as png. The scene is rendered offscreen with
 * the pinhole projection of the camera (resolution, focal length, sensor size,
 * principal point, roll), then depth of field (aperture, focus distance) and lens
 * distortion (radial k1-k6, tangential p1-p2, OpenCV camera model) are simulated with OpenCV.
 */
class CameraImageSaver
{
public:
    // renders the scene with the projection and view matrices to the current framebuffer
    typedef std::function<void(const Matrix4x4 &projection, const Matrix4x4 &view)> RenderFunction;

    static bool save(const CameraObject &camera, const RenderFunction &render,
                     float zNear, float zFar, const std::string &filename);

private:
    static bool renderImage(const CameraObject &camera, const RenderFunction &render, float zNear, float zFar,
                            const cv::Rect &area, cv::Mat *image, cv::Mat *depth);
    static void applyDepthOfField(const CameraObjectConfig &config, const cv::Mat &depth, cv::Mat *image);
    static cv::Rect getDistortedArea(const cv::Mat &cameraMatrix, const cv::Mat &distortion, int width, int height);
};

#endif // CAMERA_IMAGE_SAVER_H
