QT += core widgets opengl

CONFIG += c++17

TARGET = CNC_Controller
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/openglwidget.cpp

HEADERS += \
    include/mainwindow.h \
    include/openglwidget.h

INCLUDEPATH += include

# Windows için
win32 {
    CONFIG += windows
} 