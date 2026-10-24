#include "camera_image_saver.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <algorithm>
#include <iostream>
#include <math.h>
#include <vector>

#define MM_TO_OPENGL_UNIT           1000.0f
// largest blur radius of the depth of field in pixels
#define DEPTH_OF_FIELD_MAX_RADIUS   32.0f
// count of blur levels the depth of field is interpolated from
#define DEPTH_OF_FIELD_MAX_LEVELS   16
// rendered area for the distortion can be at most this many times the image size
#define DISTORTION_MAX_AREA_RATE    3
// points on each image edge used to find the area the distortion needs
#define DISTORTION_EDGE_POINTS      64

/*!
 * \brief CameraImageSaver::save
 * \param camera camera object to save the image of
 * \param render renders the scene
 * \param zNear near plane
 * \param zFar far plane
 * \param filename full path of the png file
 * \return true on success
 */
bool CameraImageSaver::save(const CameraObject &camera, const RenderFunction &render,
                            float zNear, float zFar, const std::string &filename)
{
    const CameraObjectConfig &config = camera.getConfig();
    const int width = static_cast<int>(lroundf(config.resolutionWidth));
    const int height = static_cast<int>(lroundf(config.resolutionHeight));
    const double fx = config.focalLength*config.resolutionWidth/config.sensorWidth;
    const double fy = config.focalLength*config.resolutionHeight/config.sensorHeight;
    const cv::Mat cameraMatrix = (cv::Mat_<double>(3, 3) <<
                                  fx, 0.0, config.principalPointX,
                                  0.0, fy, config.principalPointY,
                                  0.0, 0.0, 1.0);
    // OpenCV order is k1, k2, p1, p2, k3, k4, k5, k6
    const cv::Mat distortion = (cv::Mat_<double>(1, 8) <<
                                config.radialDistortion[0], config.radialDistortion[1],
                                config.tangentialDistortion[0], config.tangentialDistortion[1],
                                config.radialDistortion[2], config.radialDistortion[3],
                                config.radialDistortion[4], config.radialDistortion[5]);
    const bool hasDistortion = cv::countNonZero(distortion) > 0;
    // distorted image needs pixels from outside of the undistorted image, so render a larger area
    const cv::Rect area = hasDistortion ? getDistortedArea(cameraMatrix, distortion, width, height)
                                        : cv::Rect(0, 0, width, height);
    cv::Mat image, depth;

    if (!renderImage(camera, render, zNear, zFar, area, &image, &depth)) {
        return false;
    }
    applyDepthOfField(config, depth, &image);

    if (hasDistortion) {
        // undistortPoints gives for every pixel of the distorted image its place in the undistorted image
        std::vector<cv::Point2f> distortedPoints, undistortedPoints;
        distortedPoints.reserve(static_cast<size_t>(width)*static_cast<size_t>(height));
        for (int y=0;y<height;y++) {
            for (int x=0;x<width;x++) {
                distortedPoints.push_back(cv::Point2f(static_cast<float>(x), static_cast<float>(y)));
            }
        }
        cv::undistortPoints(distortedPoints, undistortedPoints, cameraMatrix, distortion, cv::noArray(), cameraMatrix);
        cv::Mat mapX(height, width, CV_32FC1), mapY(height, width, CV_32FC1);
        for (int y=0;y<height;y++) {
            for (int x=0;x<width;x++) {
                const cv::Point2f &point = undistortedPoints[static_cast<size_t>(y*width + x)];
                mapX.at<float>(y, x) = point.x - static_cast<float>(area.x);
                mapY.at<float>(y, x) = point.y - static_cast<float>(area.y);
            }
        }
        cv::Mat distorted;
        cv::remap(image, distorted, mapX, mapY, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        image = distorted;
    }

    if (!cv::imwrite(filename, image)) {
        std::cerr << "Cannot save " << filename << std::endl;
        return false;
    }
    return true;
}

/*!
 * \brief CameraImageSaver::renderImage
 * renders the area of the camera image offscreen
 * \param area area in image pixels, it can be larger than the image
 * \param image returns BGR image of the area
 * \param depth returns distance from the camera plane in opengl units for every pixel
 * \return true on success
 */
bool CameraImageSaver::renderImage(const CameraObject &camera, const RenderFunction &render, float zNear, float zFar,
                                   const cv::Rect &area, cv::Mat *image, cv::Mat *depth)
{
    GLint oldFramebuffer, oldViewport[4], oldPackAlignment;
    GLfloat oldClearColor[4];
    GLuint framebuffer, renderbuffers[2];
    bool success = true;

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    glGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor);

    glGenFramebuffers(1, &framebuffer);
    glGenRenderbuffers(2, renderbuffers);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, area.width, area.height);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, area.width, area.height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffers[0]);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffers[1]);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Cannot create framebuffer " << area.width << "x" << area.height << std::endl;
        success = false;
    } else {
        // OpenCV pixel centers are at whole numbers, so the edge of the first pixel is at -0.5
        const Matrix4x4 projection = camera.getImageProjectionMatrix(static_cast<float>(area.x) - 0.5f,
                                                                     static_cast<float>(area.y) - 0.5f,
                                                                     static_cast<float>(area.width),
                                                                     static_cast<float>(area.height),
                                                                     zNear, zFar);
        glViewport(0, 0, area.width, area.height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render(projection, camera.getViewMatrix());

        cv::Mat flippedImage(area.height, area.width, CV_8UC3);
        cv::Mat flippedDepth(area.height, area.width, CV_32FC1);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, area.width, area.height, GL_BGR, GL_UNSIGNED_BYTE, flippedImage.data);
        glReadPixels(0, 0, area.width, area.height, GL_DEPTH_COMPONENT, GL_FLOAT, flippedDepth.data);
        // opengl rows start from the bottom
        cv::flip(flippedImage, *image, 0);
        cv::flip(flippedDepth, *depth, 0);

        // depth buffer to distance, projection gives z/w = a + b/z and depth buffer is (z/w + 1)/2
        const float a = zFar/(zFar - zNear);
        const float b = -zNear*zFar/(zFar - zNear);
        depth->forEach<float>([a, b, zFar](float &value, const int *) {
            const float ndc = value*2.0f - 1.0f;
            value = ndc >= 1.0f ? zFar : b/(ndc - a);
        });
    }

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(oldFramebuffer));
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteRenderbuffers(2, renderbuffers);
    glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    glPixelStorei(GL_PACK_ALIGNMENT, oldPackAlignment);
    glClearColor(oldClearColor[0], oldClearColor[1], oldClearColor[2], oldClearColor[3]);
    return success;
}

/*!
 * \brief CameraImageSaver::applyDepthOfField
 * blurs the image by the circle of confusion of the thin lens:
 * diameter on the sensor = focalLength^2/(aperture*(focusDistance - focalLength)) * |distance - focusDistance|/distance.
 * Image is blurred with disc kernels of some radiuses and every pixel is interpolated
 * from the two closest radiuses.
 * \param config aperture, focus distance, focal length and sensor size
 * \param depth distance of every pixel in opengl units
 * \param image image to blur
 */
void CameraImageSaver::applyDepthOfField(const CameraObjectConfig &config, const cv::Mat &depth, cv::Mat *image)
{
    const float focalLength = config.focalLength;
    const float focusDistance = config.focusDistance*MM_TO_OPENGL_UNIT;
    if (config.aperture <= 0.0f || focusDistance <= focalLength) {
        return;
    }
    const float pixelsPerMm = config.resolutionWidth/config.sensorWidth;
    const float maxDiameter = focalLength*focalLength/(config.aperture*(focusDistance - focalLength))*pixelsPerMm;
    cv::Mat radius(depth.size(), CV_32FC1);
    double maxRadius;

    for (int y=0;y<depth.rows;y++) {
        for (int x=0;x<depth.cols;x++) {
            const float distance = depth.at<float>(y, x)*MM_TO_OPENGL_UNIT;
            radius.at<float>(y, x) = std::min(maxDiameter*fabsf(distance - focusDistance)/distance/2.0f,
                                              DEPTH_OF_FIELD_MAX_RADIUS);
        }
    }
    cv::minMaxLoc(radius, nullptr, &maxRadius);
    if (maxRadius < 0.5) {
        // less than a pixel, image is sharp
        return;
    }

    const int levelCount = std::min(DEPTH_OF_FIELD_MAX_LEVELS, static_cast<int>(ceil(maxRadius)));
    std::vector<cv::Mat> listBlurred;
    cv::Mat source;
    image->convertTo(source, CV_32FC3);
    for (int level=0;level<=levelCount;level++) {
        const float levelRadius = static_cast<float>(maxRadius)*static_cast<float>(level)/static_cast<float>(levelCount);
        if (levelRadius < 0.5f) {
            listBlurred.push_back(source);
            continue;
        }
        // disc with smooth edge
        const int half = static_cast<int>(ceilf(levelRadius));
        cv::Mat kernel(2*half + 1, 2*half + 1, CV_32FC1);
        for (int y=-half;y<=half;y++) {
            for (int x=-half;x<=half;x++) {
                const float distance = sqrtf(static_cast<float>(x*x + y*y));
                kernel.at<float>(y + half, x + half) = std::max(0.0f, std::min(1.0f, levelRadius + 0.5f - distance));
            }
        }
        kernel /= cv::sum(kernel)[0];
        cv::Mat blurred;
        cv::filter2D(source, blurred, -1, kernel, cv::Point(-1, -1), 0, cv::BORDER_REPLICATE);
        listBlurred.push_back(blurred);
    }

    cv::Mat result(source.size(), CV_32FC3);
    for (int y=0;y<result.rows;y++) {
        for (int x=0;x<result.cols;x++) {
            const float levelPosition = radius.at<float>(y, x)/static_cast<float>(maxRadius)*static_cast<float>(levelCount);
            const int level0 = std::min(static_cast<int>(levelPosition), levelCount);
            const int level1 = std::min(level0 + 1, levelCount);
            const float rate = levelPosition - static_cast<float>(level0);
            result.at<cv::Vec3f>(y, x) = listBlurred[static_cast<size_t>(level0)].at<cv::Vec3f>(y, x)*(1.0f - rate)
                                       + listBlurred[static_cast<size_t>(level1)].at<cv::Vec3f>(y, x)*rate;
        }
    }
    result.convertTo(*image, CV_8UC3);
}

/*!
 * \brief CameraImageSaver::getDistortedArea
 * finds the area of the undistorted image the distorted image uses
 * \return area in image pixels, at least the image itself
 */
cv::Rect CameraImageSaver::getDistortedArea(const cv::Mat &cameraMatrix, const cv::Mat &distortion, int width, int height)
{
    std::vector<cv::Point2f> edgePoints, undistortedPoints;
    for (int i=0;i<=DISTORTION_EDGE_POINTS;i++) {
        const float rate = static_cast<float>(i)/static_cast<float>(DISTORTION_EDGE_POINTS);
        const float x = rate*static_cast<float>(width - 1);
        const float y = rate*static_cast<float>(height - 1);
        edgePoints.push_back(cv::Point2f(x, 0.0f));
        edgePoints.push_back(cv::Point2f(x, static_cast<float>(height - 1)));
        edgePoints.push_back(cv::Point2f(0.0f, y));
        edgePoints.push_back(cv::Point2f(static_cast<float>(width - 1), y));
    }
    cv::undistortPoints(edgePoints, undistortedPoints, cameraMatrix, distortion, cv::noArray(), cameraMatrix);

    float left = 0.0f, top = 0.0f;
    float right = static_cast<float>(width - 1), bottom = static_cast<float>(height - 1);
    for (const cv::Point2f &point : undistortedPoints) {
        left = std::min(left, point.x);
        top = std::min(top, point.y);
        right = std::max(right, point.x);
        bottom = std::max(bottom, point.y);
    }
    // limit the size by the image size and by the largest framebuffer
    GLint maxRenderbufferSize, maxViewport[2];
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewport);
    const float maxWidth = static_cast<float>(std::min({ DISTORTION_MAX_AREA_RATE*width, maxRenderbufferSize, maxViewport[0] }));
    const float maxHeight = static_cast<float>(std::min({ DISTORTION_MAX_AREA_RATE*height, maxRenderbufferSize, maxViewport[1] }));
    const float centerX = static_cast<float>(width - 1)/2.0f;
    const float centerY = static_cast<float>(height - 1)/2.0f;
    left = std::max(left, centerX - maxWidth/2.0f + 1.0f);
    right = std::min(right, centerX + maxWidth/2.0f - 1.0f);
    top = std::max(top, centerY - maxHeight/2.0f + 1.0f);
    bottom = std::min(bottom, centerY + maxHeight/2.0f - 1.0f);

    // one pixel more on every side for the linear interpolation
    const int x0 = static_cast<int>(floorf(left)) - 1;
    const int y0 = static_cast<int>(floorf(top)) - 1;
    const int x1 = static_cast<int>(ceilf(right)) + 1;
    const int y1 = static_cast<int>(ceilf(bottom)) + 1;
    return cv::Rect(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}
