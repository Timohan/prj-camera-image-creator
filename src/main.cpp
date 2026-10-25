/*!
 * \file
 * \brief file main.cpp
 *
 * Main of opengl example via glfw
 *
 * Copyright of Timo Hannukkala. All rights reserved.
 *
 * \author Timo Hannukkala <timo.hannukkala@hotmail.com>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <application/tg_application.h>
#include "mainwindow.h"

static TgApplication m_application;

TgApplication *getApplication()
{
    return &m_application;
}
/*!
 * \brief main
 * \param argc
 * \param argv --takePictures saves the pictures of the cameras like the key s and exits
 * \return
 */
int main(int argc , char *argv[])
{
    bool takePictures = false;
    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i], "--takePictures") == 0) {
            takePictures = true;
        } else {
            fprintf(stderr, "Unknown argument %s, usage: prj-camera-image-creator [--takePictures]\n", argv[i]);
            return 1;
        }
    }
    static MainWindow m_mainwindow(1920, 1080, takePictures);
    return m_application.exec();
}
