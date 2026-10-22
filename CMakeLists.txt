cmake_minimum_required(VERSION 3.16)

project(QtMaps VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 5.13 REQUIRED COMPONENTS
    Core
    Gui
    Location
    Positioning
    Qml
    Quick
)

qt5_add_resources(QTMAPS_RESOURCES resources.qrc)

add_executable(qtmaps
    app/main.cpp
    ${QTMAPS_RESOURCES}
)

target_link_libraries(qtmaps PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Location
    Qt5::Positioning
    Qt5::Qml
    Qt5::Quick
)
