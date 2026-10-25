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
#include <string>
#include "mainwindow.h"

#define OBJECTS_FILE_ARGUMENT   "--objectsFile="

static TgApplication m_application;

TgApplication *getApplication()
{
    return &m_application;
}
/*!
 * \brief main
 * \param argc
 * \param argv --takePictures saves the pictures of the cameras like the key s and exits,
 * --objectsFile=<objects file> uses the objects file instead of objectsFile of general.ini
 * \return
 */
int main(int argc , char *argv[])
{
    bool takePictures = false;
    std::string objectsFile;
    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i], "--takePictures") == 0) {
            takePictures = true;
        } else if (strncmp(argv[i], OBJECTS_FILE_ARGUMENT, strlen(OBJECTS_FILE_ARGUMENT)) == 0
                   && argv[i][strlen(OBJECTS_FILE_ARGUMENT)] != '\0') {
            objectsFile = argv[i] + strlen(OBJECTS_FILE_ARGUMENT);
        } else {
            fprintf(stderr, "Unknown argument %s, usage: prj-camera-image-creator [--takePictures] [--objectsFile=<objects file>]\n", argv[i]);
            return 1;
        }
    }
    static MainWindow m_mainwindow(1920, 1080, takePictures, objectsFile);
    return m_application.exec();
}
