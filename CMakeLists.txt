cmake_minimum_required(VERSION 3.5)
project(Saber)

set(CMAKE_CXX_STANDARD 14)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)


FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)

set(generated_mach_interfaces
        ${CMAKE_CURRENT_BINARY_DIR}/mach_exc.h
        ${CMAKE_CURRENT_BINARY_DIR}/mach_excServer.c
        ${CMAKE_CURRENT_BINARY_DIR}/mach_excUser.c
        )
add_custom_command(OUTPUT ${generated_mach_interfaces}
        COMMAND mig ${CMAKE_CURRENT_SOURCE_DIR}/dbgnub-mig.defs
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dbgnub-mig.defs
        )

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
        resource.qrc

        Breakpoint.cpp
        DebugCore.cpp
        DisasmView.cpp
        libasmx64.cpp
        TargetException.cpp
        TargetException.h
        EventDispatcher.cpp
        global.cpp
        AttachProcessList.cpp
        AttachProcessList.h
        BreakpointView.cpp
        MemoryMapView.cpp
        OutputView.cpp
        RegisterView.cpp
        MemoryView.cpp
        ${generated_mach_interfaces})

include_directories(
        ${Qt5Widgets_INCLUDES}
        ${Qt5Widgets_PRIVATE_INCLUDE_DIRS}
        ${Qt5Gui_INCLUDE_DIRS}
        ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/QtFlex5
        ${CMAKE_SOURCE_DIR}/qhexview
)


add_executable(Saber ${SOURCE_FILES})

target_link_libraries(Saber Qt5::Widgets Qt5::Gui)
