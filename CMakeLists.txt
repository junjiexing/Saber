cmake_minimum_required(VERSION 3.5)
project(Saber)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)


FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)

set(SOURCE_FILES
        main.cpp
        MainWindow.cpp
        QtFlex5/QtDockGuider.cpp
        QtFlex5/QtDockSide.cpp
        QtFlex5/QtDockSite.cpp
        QtFlex5/QtDockWidget.cpp
        QtFlex5/QtFlexHelper.cpp
        QtFlex5/QtFlexManager.cpp
        QtFlex5/QtFlexStyle.cpp
        QtFlex5/QtFlexWidget.cpp
        QtFlex5/QtFlex.qrc

        Breakpoint.cpp
        DebugCore.cpp
        DisasmView.cpp
        libasmx64.cpp
        RegisterModel.cpp

        mach_excServer.c
        mach_excUser.c
        TargetException.cpp TargetException.h Log.cpp Log.h EventDispatcher.cpp EventDispatcher.h)

include_directories(
        ${Qt5Widgets_INCLUDES}
        ${Qt5Widgets_PRIVATE_INCLUDE_DIRS}
        ${Qt5Gui_INCLUDE_DIRS}
        ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/QtFlex5
)


add_executable(Saber ${SOURCE_FILES})

target_link_libraries(Saber Qt5::Widgets Qt5::Gui)
