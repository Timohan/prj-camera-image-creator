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
 * Distance from the camera to every pixel, camera and lens information and the surface of every pixel
 * (surface number and normal) are saved to binary data file, see saveData() for the format.
 */
class CameraImageSaver
{
public:
    // renders the scene with the projection and view matrices to the current framebuffer,
    // if distanceFrom is not nullptr, renders distance from it to every pixel and no sky,
    // with surfacePass the normal toward distanceFrom (rgb) and the surface number (alpha) instead of the distance
    typedef std::function<void(const Matrix4x4 &projection, const Matrix4x4 &view,
                               const VECTOR3 *distanceFrom, bool surfacePass)> RenderFunction;

    static bool save(const CameraObject &camera, const RenderFunction &render, float zNear, float zFar, int supersampling,
                     int cameraNumber, const std::string &imageFilename, const std::string &dataFilename);
    static CameraObjectConfig getScaledConfig(const CameraObjectConfig &config, float scale);

private:
    static bool createPinholeImage(const CameraObject &camera, const RenderFunction &render,
                                   float zNear, float zFar, cv::Mat *image, cv::Mat *distance, cv::Mat *surface);
    static bool createFisheyeImage(const CameraObject &camera, const RenderFunction &render,
                                   float zNear, float zFar, cv::Mat *image, cv::Mat *distance, cv::Mat *surface);
    static bool renderImage(const RenderFunction &render, const Matrix4x4 &projection, const Matrix4x4 &view,
                            const VECTOR3 &position, float zNear, float zFar, int width, int height,
                            cv::Mat *image, cv::Mat *depth, cv::Mat *distance, cv::Mat *surface);
    static bool saveData(const CameraObject &camera, int cameraNumber, const cv::Mat &distance, const cv::Mat &surface,
                         const std::string &filename);
    static cv::Vec3b sampleLinear(const cv::Mat &image, float column, float row);
    static void shrinkImage(int supersampling, cv::Mat *image, cv::Mat *distance, cv::Mat *surface);
    static void applyDepthOfField(const CameraObjectConfig &config, const cv::Mat &depth, cv::Mat *image);
    static cv::TermCriteria getUndistortCriteria();
    static cv::Rect getDistortedArea(const cv::Mat &cameraMatrix, const cv::Mat &distortion, int width, int height);
};

#endif // CAMERA_IMAGE_SAVER_H
