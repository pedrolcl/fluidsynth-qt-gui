# SPDX-License-Identifier: MIT
# Copyright (c) 2024 Pedro López-Cabanillas <plcl@users.sf.net>

cmake_minimum_required(VERSION 3.19)

project(fluidsynth-qt-gui
    VERSION 0.0.1
    LANGUAGES CXX
)

find_package(QT NAMES Qt6 REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)
find_package(FluidSynth REQUIRED)

qt_standard_project_setup()

qt_add_executable( ${PROJECT_NAME}
    ConsoleIODevice.cpp
    ConsoleIODevice.h
    ConsoleWidget.cpp
    ConsoleWidget.h
    fluidcompleter.cpp
    fluidcompleter.h
    fluidsynthwrapper.cpp
    fluidsynthwrapper.h
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries( ${PROJECT_NAME} PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
    FluidSynth::libfluidsynth
)

target_compile_definitions( ${PROJECT_NAME} PRIVATE
    VERSION=${PROJECT_VERSION}
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
