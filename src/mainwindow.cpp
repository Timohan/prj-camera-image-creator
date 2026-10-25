#include "mainwindow.h"
#include <iostream>

MainWindow::MainWindow(int width, int height, bool takePictures, const std::string &objectsFile)
    : TgMainWindow(width, height, "prj-camera-image-creator"),
    m_baseScene(objectsFile),
    m_cameraKeyListener(this),
    m_takePictures(takePictures),
    m_picturesRequested(false)
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
    m_cameraKeyListener.connectOnPitchStart([this](int direction) {
        m_baseScene.startPitch(direction);
    });
    m_cameraKeyListener.connectOnPitchStop([this](int direction) {
        m_baseScene.stopPitch(direction);
    });
    m_cameraKeyListener.connectOnMove([this](float steps) {
        m_baseScene.moveCamera(steps);
    });
}

int MainWindow::initCustomShader()
{
    return m_baseScene.initCustomShader();
}

void MainWindow::customBeforeRender()
{
    if (m_takePictures && !m_picturesRequested) {
        m_picturesRequested = true;
        m_baseScene.requestSaveImages();
        requestRender();
    }
    if (m_baseScene.updateTurn()) {
        // renders continuously while turning, otherwise next frame waits for events
        requestRender();
    }
}

void MainWindow::customRender()
{
    m_baseScene.customRender(getWidth(), getHeight());
    if (m_takePictures && m_picturesRequested) {
        // the pictures were saved by the render above
        exit();
    }
}
