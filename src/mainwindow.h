#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <window/tg_mainwindow.h>
#include "scene/base_scene.h"

class MainWindow : public TgMainWindow
{
public:
    MainWindow(int width, int height);

protected:
    virtual int initCustomShader() override;
    virtual void customRender() override;

private:
    BaseScene m_baseScene;
};

#endif
