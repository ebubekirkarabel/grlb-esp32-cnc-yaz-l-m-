QT += core widgets opengl serialport

CONFIG += c++17

TARGET = CNC_Controller
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/openglwidget.cpp \
    src/gcodeparser.cpp \
    src/serialcommunication.cpp \
    src/axiscontroller.cpp \
    src/settings.cpp \
    src/logger.cpp

HEADERS += \
    include/mainwindow.h \
    include/openglwidget.h \
    include/gcodeparser.h \
    include/serialcommunication.h \
    include/axiscontroller.h \
    include/settings.h \
    include/logger.h

INCLUDEPATH += include

# Windows için
win32 {
    CONFIG += windows
} 