#include "mainwindow.h"
#include <iostream>

// labels at the top left corner: the picked pixel (same as in prj-camera-image-viewer), the hidden areas and
// the cameras
#define LABEL_MARGIN                10
#define LABEL_HEIGHT                36
#define LABEL_FONT_SIZE             20
#define PIXEL_LABEL_WIDTH           500
#define HIDDEN_LABEL_WIDTH          260
#define CAMERAS_LABEL_WIDTH         260
#define LABEL_BACKGROUND_COLOR      30, 80, 200, 255
#define LABEL_TEXT_COLOR            255, 255, 255
#define PIXEL_LABEL_START_TEXT      "Click a pixel"

MainWindow::MainWindow(int width, int height, bool takePictures, const std::string &objectsFile, int camerasVisible)
    : TgMainWindow(width, height, "prj-camera-image-creator"),
    m_baseScene(objectsFile, camerasVisible),
    m_mouseCapture(this),
    m_cameraKeyListener(this),
    m_pixelBackground(this, LABEL_MARGIN, LABEL_MARGIN, PIXEL_LABEL_WIDTH, LABEL_HEIGHT, LABEL_BACKGROUND_COLOR),
    m_pixelText(&m_pixelBackground, PIXEL_LABEL_START_TEXT, "", LABEL_FONT_SIZE, LABEL_TEXT_COLOR),
    m_hiddenBackground(this, 2*LABEL_MARGIN + PIXEL_LABEL_WIDTH, LABEL_MARGIN, HIDDEN_LABEL_WIDTH, LABEL_HEIGHT,
                       LABEL_BACKGROUND_COLOR),
    m_hiddenText(&m_hiddenBackground, "", "", LABEL_FONT_SIZE, LABEL_TEXT_COLOR),
    m_camerasBackground(this, 3*LABEL_MARGIN + PIXEL_LABEL_WIDTH + HIDDEN_LABEL_WIDTH, LABEL_MARGIN, CAMERAS_LABEL_WIDTH,
                        LABEL_HEIGHT, LABEL_BACKGROUND_COLOR),
    m_camerasText(&m_camerasBackground, "", "", LABEL_FONT_SIZE, LABEL_TEXT_COLOR),
    m_takePictures(takePictures),
    m_picturesRequested(false)
{
    m_pixelText.setHorizontalAlign(TgTextfieldHorizontalAlign::AlignCenterH);
    m_pixelText.setVerticalAlign(TgTextfieldVerticalAlign::AlignCenterV);
    m_hiddenText.setHorizontalAlign(TgTextfieldHorizontalAlign::AlignCenterH);
    m_hiddenText.setVerticalAlign(TgTextfieldVerticalAlign::AlignCenterV);
    updateHiddenText();
    m_camerasText.setHorizontalAlign(TgTextfieldHorizontalAlign::AlignCenterH);
    m_camerasText.setVerticalAlign(TgTextfieldVerticalAlign::AlignCenterV);
    updateCamerasText();
    m_mouseCapture.connectOnMouseClicked([this](TgMouseType button, float x, float y, const void *) {
        if (button == TgMouseType::ButtonLeft) {
            m_baseScene.pick(x, y);
            requestRender();
        }
    });
    m_cameraKeyListener.connectOnCameraSelected([this](int index) {
        m_baseScene.moveToCamera(index);
    });
    m_cameraKeyListener.connectOnToggleDirectionLine([this]() {
        m_baseScene.toggleDirectionLine();
    });
    m_cameraKeyListener.connectOnToggleCameraObjects([this]() {
        m_baseScene.toggleCameraObjects();
        updateCamerasText();
    });
    m_cameraKeyListener.connectOnSaveImages([this](bool withData) {
        m_baseScene.requestSaveImages(withData);
    });
    m_cameraKeyListener.connectOnToggleHideUnseen([this]() {
        m_baseScene.toggleHideUnseen();
        updateHiddenText();
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
    if (m_baseScene.takePicked()) {
        m_pixelText.setText(m_baseScene.getPickedText().c_str());
        requestRender();
    }
    if (m_takePictures && m_picturesRequested) {
        // the pictures were saved by the render above
        exit();
    }
}

void MainWindow::updateHiddenText()
{
    m_hiddenText.setText(m_baseScene.getHideUnseen() ? "Hidden areas: black" : "Hidden areas: shown");
}

void MainWindow::updateCamerasText()
{
    m_camerasText.setText(m_baseScene.getCameraObjectsVisible() ? "Cameras: shown" : "Cameras: hidden");
}
