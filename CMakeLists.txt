cmake_minimum_required(VERSION 3.16)

project(CNC_Controller VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 bulma
find_package(Qt6 REQUIRED COMPONENTS Core Widgets OpenGL SerialPort)

# Kaynak dosyalar
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/openglwidget.cpp
    src/gcodeparser.cpp
    src/serialcommunication.cpp
    src/axiscontroller.cpp
    src/settings.cpp
    src/logger.cpp
)

set(HEADERS
    include/mainwindow.h
    include/openglwidget.h
    include/gcodeparser.h
    include/serialcommunication.h
    include/axiscontroller.h
    include/settings.h
    include/logger.h
)

# Executable oluşturma
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Qt modüllerini bağlama
target_link_libraries(${PROJECT_NAME} 
    Qt6::Core 
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::SerialPort
)

# Include dizinleri
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Windows için icon ve metadata
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif() 