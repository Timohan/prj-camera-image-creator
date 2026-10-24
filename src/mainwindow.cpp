#include "mainwindow.h"
#include <iostream>

MainWindow::MainWindow(int width, int height)
    : TgMainWindow(width, height, "prj-camera-image-creator"),
    m_cameraKeyListener(this)
{
    m_cameraKeyListener.connectOnCameraSelected([this](int index) {
        m_baseScene.moveToCamera(index);
    });
    m_cameraKeyListener.connectOnToggleDirectionLine([this]() {
        m_baseScene.toggleDirectionLine();
    });
    m_cameraKeyListener.connectOnToggleCameraObjects([this]() {
        m_baseScene.toggleCameraObjects();
    });
    m_cameraKeyListener.connectOnSaveImages([this]() {
        m_baseScene.requestSaveImages();
    });
    m_cameraKeyListener.connectOnTurnStart([this](int direction) {
        m_baseScene.startTurn(direction);
    });
    m_cameraKeyListener.connectOnTurnStop([this](int direction) {
        m_baseScene.stopTurn(direction);
    });
}

int MainWindow::initCustomShader()
{
    return m_baseScene.initCustomShader();
}

void MainWindow::customBeforeRender()
{
    if (m_baseScene.updateTurn()) {
        // renders continuously while turning, otherwise next frame waits for events
        requestRender();
    }
}

void MainWindow::customRender()
{
    m_baseScene.customRender(getWidth(), getHeight());
}
