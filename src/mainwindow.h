#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <window/tg_mainwindow.h>
#include "scene/base_scene.h"
#include "scene/camera_key_listener.h"

class MainWindow : public TgMainWindow
{
public:
    // takePictures: saves the pictures of the cameras at the start like the key s and exits,
    // objectsFile: file in objectsDirectory of general.ini, if it is empty, objectsFile of general.ini is used,
    // camerasVisible: 0 or 1, if it is -1, camerasVisible of general.ini is used
    MainWindow(int width, int height, bool takePictures = false, const std::string &objectsFile = std::string(),
               int camerasVisible = -1);

protected:
    virtual int initCustomShader() override;
    virtual void customBeforeRender() override;
    virtual void customRender() override;

private:
    BaseScene m_baseScene;
    CameraKeyListener m_cameraKeyListener;
    bool m_takePictures;
    bool m_picturesRequested;
};

#endif
