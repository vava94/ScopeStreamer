cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME ScopeStreamer)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS
        Core
        Gui
        Widgets
        Xml
        XmlPatterns
        REQUIRED)


#add_subdirectory(QOnvifManager)
add_subdirectory(CameraHandler)
add_subdirectory(ColorConverter)

add_executable(${PROJECT_NAME} main.cpp)


target_link_libraries(${PROJECT_NAME} ColorConverter CameraHandler)#QOnvifManager)
