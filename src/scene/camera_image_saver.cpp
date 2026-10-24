#include "camera_image_saver.h"
#include <GL/glew.h>
#include <GL/gl.h>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/utility.hpp>
#include <algorithm>
#include <cstdint>
#include <fstream>
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
// fisheye cube face size limits in pixels
#define FISHEYE_MIN_FACE_SIZE       256
#define FISHEYE_MAX_FACE_SIZE       4096
// cube face sees this many pixels more on every side than 90 degrees
#define FISHEYE_FACE_MARGIN_PIXELS  2.0f
#define FISHEYE_NEWTON_ITERATIONS   20
// average distance from the earth to the sun in mm, distance of the sky
#define EARTH_ORBIT_DISTANCE_MM     149597870700000.0
// distance map values that are not distances
#define DISTANCE_SKY                -1.0f
#define DISTANCE_NO_VIEW            -2.0f
// camera and lens information block after the distances in the data file
#define DATA_CAMERA_INFO_MAGIC      "CAMI"
#define DATA_CAMERA_INFO_VERSION    2

/*!
 * \brief CameraImageSaver::save
 * \param camera camera object to save the image of
 * \param render renders the scene
 * \param zNear near plane
 * \param zFar far plane
 * \param cameraNumber number of the camera (1-cameraCount), it is saved to the data file
 * \param imageFilename full path of the png file
 * \param dataFilename full path of the binary data file
 * \return true on success
 */
bool CameraImageSaver::save(const CameraObject &camera, const RenderFunction &render, float zNear, float zFar,
                            int cameraNumber, const std::string &imageFilename, const std::string &dataFilename)
{
    cv::Mat image, distance;
    const bool success = camera.getConfig().fisheye ? createFisheyeImage(camera, render, zNear, zFar, &image, &distance)
                                                    : createPinholeImage(camera, render, zNear, zFar, &image, &distance);
    if (!success) {
        return false;
    }
    if (!cv::imwrite(imageFilename, image)) {
        std::cerr << "Cannot save " << imageFilename << std::endl;
        return false;
    }
    return saveData(camera, cameraNumber, distance, dataFilename);
}

/*!
 * \brief CameraImageSaver::saveData
 * saves binary data file (native byte order, little-endian on x86):
 * uint32_t width, uint32_t height,
 * double distance in mm for every pixel row by row from the top left pixel, sky is as far
 * as earth's orbit and pixels that do not see anything (outside of the lens field of view) are 0,
 * then camera and lens information:
 * char[4] "CAMI", uint32_t version (2), uint32_t camera number, uint32_t distortion model (0 pinhole, 1 fisheye),
 * and doubles: position x/y/z (mm), direction x/y/z, up vector x/y/z (with roll), roll (degrees),
 * focal length (mm), sensor width/height (mm), resolution width/height (pixels),
 * focal length x/y (pixels), principal point x/y (pixels), radial distortion k1-k6,
 * tangential distortion p1-p2, fisheye distortion k1-k4, field of view (degrees),
 * aperture (f-number), focus distance (mm), yaw (degrees, 0 is +z, positive turns to +x),
 * pitch (degrees, positive is up).
 * World coordinates: x right, y up, z forward (left-handed).
 * \param camera camera object
 * \param cameraNumber number of the camera
 * \param distance distance of every pixel in opengl units, DISTANCE_SKY or DISTANCE_NO_VIEW
 * \param filename full path of the data file
 * \return true on success
 */
bool CameraImageSaver::saveData(const CameraObject &camera, int cameraNumber, const cv::Mat &distance, const std::string &filename)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot save " << filename << std::endl;
        return false;
    }
    auto writeUint32 = [&file](uint32_t value) {
        file.write(reinterpret_cast<const char *>(&value), sizeof(value));
    };
    auto writeDouble = [&file](double value) {
        file.write(reinterpret_cast<const char *>(&value), sizeof(value));
    };
    auto writeVector = [&writeDouble](const VECTOR3 &vector, double multiplier) {
        writeDouble(static_cast<double>(vector.x)*multiplier);
        writeDouble(static_cast<double>(vector.y)*multiplier);
        writeDouble(static_cast<double>(vector.z)*multiplier);
    };

    writeUint32(static_cast<uint32_t>(distance.cols));
    writeUint32(static_cast<uint32_t>(distance.rows));
    std::vector<double> row(static_cast<size_t>(distance.cols));
    for (int y=0;y<distance.rows;y++) {
        for (int x=0;x<distance.cols;x++) {
            const float value = distance.at<float>(y, x);
            double millimeters = 0.0;
            if (value == DISTANCE_SKY) {
                millimeters = EARTH_ORBIT_DISTANCE_MM;
            } else if (value >= 0.0f) {
                millimeters = static_cast<double>(value)*MM_TO_OPENGL_UNIT;
            }
            row[static_cast<size_t>(x)] = millimeters;
        }
        file.write(reinterpret_cast<const char *>(row.data()), static_cast<std::streamsize>(row.size()*sizeof(double)));
    }

    const CameraObjectConfig &config = camera.getConfig();
    file.write(DATA_CAMERA_INFO_MAGIC, 4);
    writeUint32(DATA_CAMERA_INFO_VERSION);
    writeUint32(static_cast<uint32_t>(cameraNumber));
    writeUint32(config.fisheye ? 1 : 0);
    writeVector(camera.getPosition(), MM_TO_OPENGL_UNIT);
    writeVector(camera.getDirection(), 1.0);
    writeVector(camera.getUpVector(), 1.0);
    writeDouble(config.roll);
    writeDouble(config.focalLength);
    writeDouble(config.sensorWidth);
    writeDouble(config.sensorHeight);
    writeDouble(config.resolutionWidth);
    writeDouble(config.resolutionHeight);
    writeDouble(static_cast<double>(config.focalLength)*config.resolutionWidth/config.sensorWidth);
    writeDouble(static_cast<double>(config.focalLength)*config.resolutionHeight/config.sensorHeight);
    writeDouble(config.principalPointX);
    writeDouble(config.principalPointY);
    for (float k : config.radialDistortion) {
        writeDouble(k);
    }
    for (float p : config.tangentialDistortion) {
        writeDouble(p);
    }
    for (float k : config.fisheyeDistortion) {
        writeDouble(k);
    }
    writeDouble(config.fieldOfView);
    writeDouble(config.aperture);
    writeDouble(static_cast<double>(config.focusDistance)*MM_TO_OPENGL_UNIT);
    const VECTOR3 direction = camera.getDirection();
    writeDouble(atan2(static_cast<double>(direction.x), static_cast<double>(direction.z))*180.0/M_PI);
    writeDouble(asin(std::max(-1.0, std::min(1.0, static_cast<double>(direction.y))))*180.0/M_PI);

    if (!file.good()) {
        std::cerr << "Cannot save " << filename << std::endl;
        return false;
    }
    return true;
}

/*!
 * \brief CameraImageSaver::createPinholeImage
 * creates the image of the OpenCV pinhole camera model with radial (k1-k6) and tangential (p1-p2) distortion
 * \param image returns the BGR image
 * \param distance returns distance of every pixel
 * \return true on success
 */
bool CameraImageSaver::createPinholeImage(const CameraObject &camera, const RenderFunction &render,
                                          float zNear, float zFar, cv::Mat *image, cv::Mat *distance)
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
    cv::Mat depth;

    // OpenCV pixel centers are at whole numbers, so the edge of the first pixel is at -0.5
    const Matrix4x4 projection = camera.getImageProjectionMatrix(static_cast<float>(area.x) - 0.5f,
                                                                 static_cast<float>(area.y) - 0.5f,
                                                                 static_cast<float>(area.width),
                                                                 static_cast<float>(area.height),
                                                                 zNear, zFar);
    if (!renderImage(render, projection, camera.getViewMatrix(), camera.getPosition(), zNear, zFar,
                     area.width, area.height, image, &depth, distance)) {
        return false;
    }
    applyDepthOfField(config, depth, image);

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
        cv::Mat distorted, distortedDistance;
        cv::remap(*image, distorted, mapX, mapY, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        // distances of different objects must not be mixed
        cv::remap(*distance, distortedDistance, mapX, mapY, cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(DISTANCE_NO_VIEW));
        *image = distorted;
        *distance = distortedDistance;
    }
    return true;
}

/*!
 * \brief CameraImageSaver::createFisheyeImage
 * creates the image of the OpenCV fisheye camera model (k1-k4):
 * distorted angle theta_d = theta*(1 + k1*theta^2 + k2*theta^4 + k3*theta^6 + k4*theta^8),
 * pixel = focal length in pixels * theta_d * (direction on the image) + principal point.
 * Fisheye can see 180 degrees or more, so the scene is rendered to 6 faces of a cube
 * around the camera and every pixel takes its color from the face its ray hits.
 * Rays wider than the field of view of the lens are black.
 * \param image returns the BGR image
 * \param distance returns distance of every pixel
 * \return true on success
 */
bool CameraImageSaver::createFisheyeImage(const CameraObject &camera, const RenderFunction &render,
                                          float zNear, float zFar, cv::Mat *image, cv::Mat *distance)
{
    const CameraObjectConfig &config = camera.getConfig();
    const int width = static_cast<int>(lroundf(config.resolutionWidth));
    const int height = static_cast<int>(lroundf(config.resolutionHeight));
    const double fx = config.focalLength*config.resolutionWidth/config.sensorWidth;
    const double fy = config.focalLength*config.resolutionHeight/config.sensorHeight;
    const double maxTheta = config.fieldOfView/2.0*M_PI/180.0;
    const double k[4] = { config.fisheyeDistortion[0], config.fisheyeDistortion[1],
                          config.fisheyeDistortion[2], config.fisheyeDistortion[3] };

    // cube face has same pixel density at its center as the fisheye image at its center,
    // face sees a bit more than 90 degrees, so the linear interpolation works on the face edges
    GLint maxRenderbufferSize, maxViewport[2];
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewport);
    const int maxFaceSize = std::min({ FISHEYE_MAX_FACE_SIZE, static_cast<int>(maxRenderbufferSize),
                                       static_cast<int>(maxViewport[0]), static_cast<int>(maxViewport[1]) });
    const int faceSize = std::max(FISHEYE_MIN_FACE_SIZE,
                                  std::min(maxFaceSize, static_cast<int>(ceil(2.0*std::max(fx, fy)))));
    const float tanHalf = 1.0f + FISHEYE_FACE_MARGIN_PIXELS*2.0f/static_cast<float>(faceSize);

    // faces: direction D and up vector U, right vector R = U x D is the right side of the face image
    const VECTOR3 forward = camera.getDirection();
    const VECTOR3 up = camera.getUpVector();
    const VECTOR3 right = Math3D::Vec3Normalize(Math3D::Vec3Cross(up, forward));
    auto negative = [](const VECTOR3 &v) { VECTOR3 n = { -v.x, -v.y, -v.z }; return n; };
    struct Face {
        VECTOR3 direction, up, right;
        cv::Mat image, depth, distance;
    };
    Face faces[6];
    const VECTOR3 faceDirections[6] = { forward, right, negative(right), negative(forward), up, negative(up) };
    const VECTOR3 faceUps[6] = { up, up, up, up, negative(forward), forward };

    Matrix4x4 projection;
    Math3D::MatrixPerspectiveFovLH(&projection, 2.0f*atanf(tanHalf), 1.0f, zNear, zFar);
    const VECTOR3 position = camera.getPosition();
    for (int i=0;i<6;i++) {
        faces[i].direction = faceDirections[i];
        faces[i].up = faceUps[i];
        faces[i].right = Math3D::Vec3Cross(faceUps[i], faceDirections[i]);
        VECTOR3 at;
        at.x = position.x + faceDirections[i].x;
        at.y = position.y + faceDirections[i].y;
        at.z = position.z + faceDirections[i].z;
        Matrix4x4 view;
        Math3D::MatrixLookAtLH(&view, position, at, faceUps[i]);
        if (!renderImage(render, projection, view, position, zNear, zFar, faceSize, faceSize,
                         &faces[i].image, &faces[i].depth, &faces[i].distance)) {
            return false;
        }
    }

    cv::Mat result(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
    // distance along the ray for the depth of field
    cv::Mat rayDistance(height, width, CV_32FC1, cv::Scalar(zFar));
    cv::Mat resultDistance(height, width, CV_32FC1, cv::Scalar(DISTANCE_NO_VIEW));
    cv::parallel_for_(cv::Range(0, height), [&](const cv::Range &rows) {
        for (int y=rows.start;y<rows.end;y++) {
            for (int x=0;x<width;x++) {
                const double xd = (static_cast<double>(x) - config.principalPointX)/fx;
                const double yd = (static_cast<double>(y) - config.principalPointY)/fy;
                const double thetaD = sqrt(xd*xd + yd*yd);
                // theta from theta_d with Newton's method
                double theta = thetaD;
                bool valid = true;
                for (int iteration=0;iteration<FISHEYE_NEWTON_ITERATIONS;iteration++) {
                    const double theta2 = theta*theta;
                    const double value = theta*(1.0 + theta2*(k[0] + theta2*(k[1] + theta2*(k[2] + theta2*k[3])))) - thetaD;
                    const double derivative = 1.0 + theta2*(3.0*k[0] + theta2*(5.0*k[1] + theta2*(7.0*k[2] + theta2*9.0*k[3])));
                    if (derivative <= 0.0) {
                        valid = false;
                        break;
                    }
                    const double step = value/derivative;
                    theta -= step;
                    if (fabs(step) < 1e-10) {
                        break;
                    }
                }
                if (!valid || theta < 0.0 || theta > maxTheta || theta > M_PI) {
                    continue;
                }
                // ray in camera coordinates (x right, y down, z forward) and in the world
                const double sinTheta = sin(theta);
                const double rayX = thetaD > 0.0 ? sinTheta*xd/thetaD : 0.0;
                const double rayY = thetaD > 0.0 ? sinTheta*yd/thetaD : 0.0;
                const double rayZ = cos(theta);
                VECTOR3 ray;
                ray.x = static_cast<float>(right.x*rayX - up.x*rayY + forward.x*rayZ);
                ray.y = static_cast<float>(right.y*rayX - up.y*rayY + forward.y*rayZ);
                ray.z = static_cast<float>(right.z*rayX - up.z*rayY + forward.z*rayZ);

                int faceIndex = 0;
                float faceDot = -2.0f;
                for (int i=0;i<6;i++) {
                    const float dot = Math3D::Vec3Dot(ray, faces[i].direction);
                    if (dot > faceDot) {
                        faceDot = dot;
                        faceIndex = i;
                    }
                }
                const Face &face = faces[faceIndex];
                const float faceX = Math3D::Vec3Dot(ray, face.right)/faceDot/tanHalf;
                const float faceY = Math3D::Vec3Dot(ray, face.up)/faceDot/tanHalf;
                const float column = (faceX + 1.0f)/2.0f*static_cast<float>(faceSize) - 0.5f;
                const float row = (1.0f - faceY)/2.0f*static_cast<float>(faceSize) - 0.5f;
                result.at<cv::Vec3b>(y, x) = sampleLinear(face.image, column, row);
                // depth of the face is along the face direction, distance is along the ray
                const int depthColumn = std::min(faceSize - 1, std::max(0, static_cast<int>(lroundf(column))));
                const int depthRow = std::min(faceSize - 1, std::max(0, static_cast<int>(lroundf(row))));
                rayDistance.at<float>(y, x) = std::min(zFar, face.depth.at<float>(depthRow, depthColumn)/faceDot);
                resultDistance.at<float>(y, x) = face.distance.at<float>(depthRow, depthColumn);
            }
        }
    });
    applyDepthOfField(config, rayDistance, &result);
    *image = result;
    *distance = resultDistance;
    return true;
}

/*!
 * \brief CameraImageSaver::sampleLinear
 * \param image BGR image
 * \param column, row position in pixels, pixel centers are at whole numbers
 * \return linearly interpolated color, positions outside of the image use the edge pixels
 */
cv::Vec3b CameraImageSaver::sampleLinear(const cv::Mat &image, float column, float row)
{
    const float x = std::min(static_cast<float>(image.cols - 1), std::max(0.0f, column));
    const float y = std::min(static_cast<float>(image.rows - 1), std::max(0.0f, row));
    const int x0 = static_cast<int>(x);
    const int y0 = static_cast<int>(y);
    const int x1 = std::min(x0 + 1, image.cols - 1);
    const int y1 = std::min(y0 + 1, image.rows - 1);
    const float rateX = x - static_cast<float>(x0);
    const float rateY = y - static_cast<float>(y0);
    cv::Vec3b color;
    for (int c=0;c<3;c++) {
        const float top = image.at<cv::Vec3b>(y0, x0)[c]*(1.0f - rateX) + image.at<cv::Vec3b>(y0, x1)[c]*rateX;
        const float bottom = image.at<cv::Vec3b>(y1, x0)[c]*(1.0f - rateX) + image.at<cv::Vec3b>(y1, x1)[c]*rateX;
        color[c] = static_cast<uchar>(lroundf(top*(1.0f - rateY) + bottom*rateY));
    }
    return color;
}

/*!
 * \brief CameraImageSaver::renderImage
 * renders the scene offscreen
 * \param projection projection matrix
 * \param view view matrix
 * \param width, height size of the image in pixels
 * \param image returns BGR image
 * \param depth returns distance from the camera plane in opengl units for every pixel
 * \param distance returns distance from the position in opengl units for every pixel, DISTANCE_SKY for the sky,
 * it is rendered separately to float framebuffer, so it is exact also far away
 * \return true on success
 */
bool CameraImageSaver::renderImage(const RenderFunction &render, const Matrix4x4 &projection, const Matrix4x4 &view,
                                   const VECTOR3 &position, float zNear, float zFar, int width, int height,
                                   cv::Mat *image, cv::Mat *depth, cv::Mat *distance)
{
    GLint oldFramebuffer, oldViewport[4], oldPackAlignment;
    GLfloat oldClearColor[4];
    GLuint framebuffer, renderbuffers[3];
    bool success = true;

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFramebuffer);
    glGetIntegerv(GL_VIEWPORT, oldViewport);
    glGetIntegerv(GL_PACK_ALIGNMENT, &oldPackAlignment);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, oldClearColor);

    glGenFramebuffers(1, &framebuffer);
    glGenRenderbuffers(3, renderbuffers);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[0]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[1]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffers[2]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_R32F, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffers[0]);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffers[1]);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Cannot create framebuffer " << width << "x" << height << std::endl;
        success = false;
    } else {
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render(projection, view, nullptr);

        cv::Mat flippedImage(height, width, CV_8UC3);
        cv::Mat flippedDepth(height, width, CV_32FC1);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_BGR, GL_UNSIGNED_BYTE, flippedImage.data);
        glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, flippedDepth.data);
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

        // distance pass to float color buffer, pixels without objects are sky
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffers[2]);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Cannot create distance framebuffer " << width << "x" << height << std::endl;
            success = false;
        } else {
            // distances are larger than 1.0, so colors must not be clamped
            GLint oldClampFragmentColor, oldClampReadColor;
            glGetIntegerv(GL_CLAMP_FRAGMENT_COLOR, &oldClampFragmentColor);
            glGetIntegerv(GL_CLAMP_READ_COLOR, &oldClampReadColor);
            glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);
            glClampColor(GL_CLAMP_READ_COLOR, GL_FALSE);
            glClearColor(DISTANCE_SKY, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            render(projection, view, &position);
            cv::Mat flippedDistance(height, width, CV_32FC1);
            glReadPixels(0, 0, width, height, GL_RED, GL_FLOAT, flippedDistance.data);
            cv::flip(flippedDistance, *distance, 0);
            glClampColor(GL_CLAMP_FRAGMENT_COLOR, static_cast<GLenum>(oldClampFragmentColor));
            glClampColor(GL_CLAMP_READ_COLOR, static_cast<GLenum>(oldClampReadColor));
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(oldFramebuffer));
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteRenderbuffers(3, renderbuffers);
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
