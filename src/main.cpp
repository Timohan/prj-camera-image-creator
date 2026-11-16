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
#include "scene/model_mesh.h"

#define OBJECTS_FILE_ARGUMENT   "--objectsFile="
#define CAMERAS_VISIBLE_ARGUMENT    "--camerasVisible="
#define CONVERT_MODEL_ARGUMENT      "--convertModel="
#define OUTPUT_ARGUMENT             "--output="

static TgApplication m_application;

TgApplication *getApplication()
{
    return &m_application;
}
/*!
 * \brief main
 * \param argc
 * \param argv --takePictures saves the pictures of the cameras like the key s and exits,
 * --objectsFile=<objects file> uses the objects file instead of objectsFile of general.ini,
 * --camerasVisible=<0 or 1> is used instead of camerasVisible of general.ini,
 * --silent does not show the window, only together with --takePictures,
 * --convertModel=<model file> --output=<.own_obj file> writes the model (obj... read with assimp and the materials of
 * its material file) as an .own_obj file and exits (no window)
 * \return
 */
int main(int argc , char *argv[])
{
    bool takePictures = false;
    bool silent = false;
    std::string objectsFile;
    // -1: camerasVisible of general.ini is used
    int camerasVisible = -1;
    std::string convertModel, output;
    for (int i=1;i<argc;i++) {
        if (strcmp(argv[i], "--takePictures") == 0) {
            takePictures = true;
        } else if (strcmp(argv[i], "--silent") == 0) {
            silent = true;
        } else if (strncmp(argv[i], OBJECTS_FILE_ARGUMENT, strlen(OBJECTS_FILE_ARGUMENT)) == 0
                   && argv[i][strlen(OBJECTS_FILE_ARGUMENT)] != '\0') {
            objectsFile = argv[i] + strlen(OBJECTS_FILE_ARGUMENT);
        } else if (strncmp(argv[i], CONVERT_MODEL_ARGUMENT, strlen(CONVERT_MODEL_ARGUMENT)) == 0) {
            convertModel = argv[i] + strlen(CONVERT_MODEL_ARGUMENT);
        } else if (strncmp(argv[i], OUTPUT_ARGUMENT, strlen(OUTPUT_ARGUMENT)) == 0) {
            output = argv[i] + strlen(OUTPUT_ARGUMENT);
        } else if (strcmp(argv[i], CAMERAS_VISIBLE_ARGUMENT "0") == 0) {
            camerasVisible = 0;
        } else if (strcmp(argv[i], CAMERAS_VISIBLE_ARGUMENT "1") == 0) {
            camerasVisible = 1;
        } else {
            fprintf(stderr, "Unknown argument %s, usage: prj-camera-image-creator [--takePictures [--silent]] [--objectsFile=<objects file>] [--camerasVisible=<0 or 1>]\n"
                    "       prj-camera-image-creator --convertModel=<model file> --output=<.own_obj file>\n", argv[i]);
            return 1;
        }
    }
    if (!convertModel.empty() || !output.empty()) {
        if (convertModel.empty() || output.empty()) {
            fprintf(stderr, "--convertModel and --output must be given together\n");
            return 1;
        }
        return ModelMesh::convert(convertModel, output);
    }
    if (silent && !takePictures) {
        // hidden window gets no keys, so the program could not be closed
        fprintf(stderr, "--silent can be used only together with --takePictures\n");
        return 1;
    }
    static MainWindow m_mainwindow(1920, 1080, takePictures, objectsFile, camerasVisible);
    if (silent) {
        // the pictures are rendered into own framebuffers, so the window is not needed for them
        m_mainwindow.setWindowVisible(false);
    }
    return m_application.exec();
}
