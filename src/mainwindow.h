#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <window/tg_mainwindow.h>
#include <item2d/tg_mouse_capture.h>
#include <item2d/tg_rectangle.h>
#include <item2d/tg_textfield.h>
#include "scene/base_scene.h"
#include "scene/camera_key_listener.h"

class MainWindow : public TgMainWindow
{
public:
    // takePictures: saves the pictures of the cameras at the start like the key s and exits,
    // objectsFile: file in <cameraImageOutputDirectory>/camera_objects of general.ini, if it is empty, objectsFile of general.ini is used,
    // camerasVisible: 0 or 1, if it is -1, camerasVisible of general.ini is used
    MainWindow(int width, int height, bool takePictures = false, const std::string &objectsFile = std::string(),
               int camerasVisible = -1);

protected:
    virtual int initCustomShader() override;
    virtual void customBeforeRender() override;
    virtual void customRender() override;

private:
    BaseScene m_baseScene;
    // mouse on the whole window: left click picks a pixel, it is before the key listener, so the key listener
    // gets the mouse wheel first (the last child gets the mouse events first)
    TgMouseCapture m_mouseCapture;
    CameraKeyListener m_cameraKeyListener;
    // picked pixel and its distance from the camera of the view, white text on a blue background
    TgRectangle m_pixelBackground;
    TgTextfield m_pixelText;
    // shows if the areas that no camera sees are shown or black (key h)
    TgRectangle m_hiddenBackground;
    TgTextfield m_hiddenText;

    // shows if the camera objects are shown or hidden (key c)
    TgRectangle m_camerasBackground;
    TgTextfield m_camerasText;

    void updateHiddenText();
    void updateCamerasText();
    bool m_takePictures;
    bool m_picturesRequested;
};

#endif
