QT       += core gui

# On the next line, add 'openglwidgets' after 'opengl' to make this work with Qt6
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets opengl

TARGET = Smoke
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

SOURCES += \
        main.cpp \
        simulation.cpp \
        visualization.cpp \
        visualization_input.cpp \
        texture.cpp \
        legend.cpp \
        mainwindow_general.cpp \
        mainwindow_simulation.cpp \
        mainwindow_scalardata.cpp

HEADERS += \
        legendscalardata.h \
        legendvectordata.h \
        mainwindow.h \
        simulation.h \
        visualization.h \
        color.h \
        datatype.h \
        texture.h \
        colormap.h \
        legend.h \
        movingaverage.h \
        fftwf_malloc_allocator.h \
        constants.h

FORMS += \
      mainwindow.ui

# These paths are here to make the project compile on OSX, here installed using Homebrew.
# Replace these with the fftw3 path on your machine.
INCLUDEPATH += /usr/local/Cellar/fftw/3.3.8/include /opt/local/include
#LIBS += -L/usr/local/Cellar/fftw/3.3.8/lib -lfftw3f
LIBS += -L/opt/local/lib -lfftw3f

RESOURCES += \
          resources.qrc

#QMAKE_RPATHDIR += /opt/local/libexec/qt6/lib
