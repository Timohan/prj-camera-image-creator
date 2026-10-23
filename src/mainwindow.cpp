#include "mainwindow.h"
#include <iostream>

MainWindow::MainWindow(int width, int height)
    : TgMainWindow(width, height, "prj-camera-image-creator"),
    m_cameraKeyListener(this)
{
    m_cameraKeyListener.connectOnCameraSelected([this](int index) {
        m_baseScene.moveToCamera(index);
    });
}

int MainWindow::initCustomShader()
{
    return m_baseScene.initCustomShader();
}

void MainWindow::customRender()
{
    m_baseScene.customRender(getWidth(), getHeight());
}
