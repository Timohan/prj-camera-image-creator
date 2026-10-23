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
 * \param argv
 * \return
 */
int main(int argc , char *argv[])
{
    static MainWindow m_mainwindow(1920, 1080);
    return m_application.exec();
}
