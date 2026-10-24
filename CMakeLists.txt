cmake_minimum_required(VERSION 3.16)

project(QtMaps VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
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
    Test
)

find_package(Git QUIET)
set(QTMAPS_BUILD_ID "unknown")
if(Git_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE QTMAPS_BUILD_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" diff --quiet --ignore-submodules HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE QTMAPS_GIT_DIRTY
        ERROR_QUIET
    )
    if(NOT QTMAPS_GIT_DIRTY EQUAL 0)
        string(APPEND QTMAPS_BUILD_ID "+working-tree")
    endif()
endif()

add_library(qtmaps_perf STATIC
    src/perf/BenchmarkCsvWriter.cpp
    src/perf/BenchmarkCsvWriter.h
    src/perf/FrameMetrics.cpp
    src/perf/FrameMetrics.h
    src/perf/FrameStatistics.cpp
    src/perf/FrameStatistics.h
    src/perf/PerformanceMonitor.cpp
    src/perf/PerformanceMonitor.h
    src/perf/ProcessTreeSampler.cpp
    src/perf/ProcessTreeSampler.h
)
target_include_directories(qtmaps_perf PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(qtmaps_perf PUBLIC Qt5::Core Qt5::Quick)

qt5_add_resources(QTMAPS_RESOURCES resources.qrc)

add_executable(qtmaps
    app/main.cpp
    ${QTMAPS_RESOURCES}
)

target_link_libraries(qtmaps PRIVATE
    qtmaps_perf
    Qt5::Core
    Qt5::Gui
    Qt5::Location
    Qt5::Positioning
    Qt5::Qml
    Qt5::Quick
)
target_compile_definitions(qtmaps PRIVATE QTMAPS_BUILD_ID="${QTMAPS_BUILD_ID}")

include(CTest)
if(BUILD_TESTING)
    add_executable(qtmaps_perf_tests tests/perf/tst_Performance.cpp)
    target_link_libraries(qtmaps_perf_tests PRIVATE qtmaps_perf Qt5::Test)
    add_test(NAME qtmaps_perf_tests COMMAND qtmaps_perf_tests)
endif()
