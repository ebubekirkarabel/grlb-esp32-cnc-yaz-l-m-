QT += core widgets

CONFIG += c++17

TARGET = CNC_Controller
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    include/mainwindow.h

INCLUDEPATH += include

# Windows için
win32 {
    CONFIG += windows
} 