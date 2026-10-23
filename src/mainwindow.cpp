#include "mainwindow.h"
#include <iostream>

MainWindow::MainWindow(int width, int height)
    : TgMainWindow(width, height, "prj-camera-image-creator")
{
}

int MainWindow::initCustomShader()
{
    return m_baseScene.initCustomShader();
}

void MainWindow::customRender()
{
    m_baseScene.customRender(getWidth(), getHeight());
}
