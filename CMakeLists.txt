cmake_minimum_required(VERSION 3.16)

project(QtMaps VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QTMAPS_MIN_QT_VERSION "5.15.18" CACHE STRING
    "Minimum Qt 5 version; set to 5.15.13 only for the documented comparison build")

find_package(Qt5 ${QTMAPS_MIN_QT_VERSION} REQUIRED COMPONENTS
    Core
    Gui
    Location
    Network
    Positioning
    Qml
    Quick
    Sql
    Test
)

if(Qt5Core_VERSION_STRING VERSION_LESS QTMAPS_MIN_QT_VERSION)
    message(FATAL_ERROR
        "Resolved Qt Core ${Qt5Core_VERSION_STRING}, below required ${QTMAPS_MIN_QT_VERSION}. "
        "Use a separate build directory for each Qt kit.")
endif()

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
    src/perf/ProcessCsvWriter.cpp
    src/perf/ProcessCsvWriter.h
    src/perf/ProcessTreeSampler.cpp
    src/perf/ProcessTreeSampler.h
)
target_include_directories(qtmaps_perf PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(qtmaps_perf PUBLIC Qt5::Core Qt5::Quick)

add_library(qtmaps_map STATIC
    src/map/DirectoryTileSource.cpp
    src/map/DirectoryTileSource.h
    src/map/EncodedTileCache.cpp
    src/map/EncodedTileCache.h
    src/map/MapPackageManifest.cpp
    src/map/MapPackageManifest.h
    src/map/MapPackageContract.h
    src/map/MapSourceController.cpp
    src/map/MapSourceController.h
    src/map/MbTilesTileSource.cpp
    src/map/MbTilesTileSource.h
    src/map/RasterPackageManifest.cpp
    src/map/RasterPackageManifest.h
    src/map/RasterTileSource.cpp
    src/map/RasterTileSource.h
    src/map/TileSource.h
)
target_include_directories(qtmaps_map PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(qtmaps_map PUBLIC
    Qt5::Core
    Qt5::Network
    Qt5::Positioning
    Qt5::Sql
)

add_library(qtmaps_diagnostics STATIC
    src/diagnostics/CameraProbeController.cpp
    src/diagnostics/CameraProbeController.h
    src/diagnostics/SimulatedNavigationController.cpp
    src/diagnostics/SimulatedNavigationController.h
)
target_include_directories(qtmaps_diagnostics PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(qtmaps_diagnostics PUBLIC Qt5::Core Qt5::Positioning)

add_library(qtmaps_routing STATIC
    src/routing/Polyline6Decoder.cpp
    src/routing/Polyline6Decoder.h
    src/routing/Route.h
    src/routing/RouteNavigationController.cpp
    src/routing/RouteNavigationController.h
    src/routing/RoutingController.cpp
    src/routing/RoutingController.h
    src/routing/ValhallaRouteClient.cpp
    src/routing/ValhallaRouteClient.h
    src/routing/ValhallaRouteParser.cpp
    src/routing/ValhallaRouteParser.h
    src/routing/ValhallaServiceManager.cpp
    src/routing/ValhallaServiceManager.h
)
target_include_directories(qtmaps_routing PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(qtmaps_routing PUBLIC Qt5::Core Qt5::Network Qt5::Positioning)

add_library(qtmaps_app_core STATIC
    src/core/ApplicationShutdownCoordinator.cpp
    src/core/ApplicationShutdownCoordinator.h
)
target_include_directories(qtmaps_app_core PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(qtmaps_app_core PUBLIC qtmaps_routing Qt5::Core)

add_library(qtmaps_benchmark STATIC
    src/benchmark/BenchmarkScenarioController.cpp
    src/benchmark/BenchmarkScenarioController.h
)
target_include_directories(qtmaps_benchmark PUBLIC "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(qtmaps_benchmark PUBLIC
    qtmaps_perf
    qtmaps_routing
    Qt5::Core
    Qt5::Positioning
    Qt5::Quick
)

qt5_add_resources(QTMAPS_QML_RESOURCES qml/resources.qrc)
qt5_add_resources(QTMAPS_ROUTING_RESOURCES resources/routing.qrc)
qt5_add_resources(QTMAPS_RASTER_RENDERER_RESOURCES tools/raster-renderer/raster_renderer.qrc)
set(QTMAPS_RESOURCES
    ${QTMAPS_QML_RESOURCES}
    ${QTMAPS_ROUTING_RESOURCES}
)

add_executable(qtmaps
    app/main.cpp
    ${QTMAPS_RESOURCES}
)

target_link_libraries(qtmaps PRIVATE
    qtmaps_app_core
    qtmaps_benchmark
    qtmaps_diagnostics
    qtmaps_map
    qtmaps_perf
    qtmaps_routing
    Qt5::Core
    Qt5::Gui
    Qt5::Location
    Qt5::Network
    Qt5::Positioning
    Qt5::Qml
    Qt5::Quick
    Qt5::Sql
)
target_compile_definitions(qtmaps PRIVATE QTMAPS_BUILD_ID="${QTMAPS_BUILD_ID}")

add_executable(qtmaps_raster_renderer
    tools/raster-renderer/main.cpp
    ${QTMAPS_RASTER_RENDERER_RESOURCES}
)
target_link_libraries(qtmaps_raster_renderer PRIVATE
    qtmaps_map
    Qt5::Core
    Qt5::Gui
    Qt5::Location
    Qt5::Positioning
    Qt5::Qml
    Qt5::Quick
)

include(CTest)
if(BUILD_TESTING)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    add_executable(qtmaps_benchmark_tests
        tests/benchmark/tst_BenchmarkScenario.cpp
        ${QTMAPS_RESOURCES}
    )
    target_link_libraries(qtmaps_benchmark_tests PRIVATE
        qtmaps_benchmark
        qtmaps_perf
        qtmaps_routing
        Qt5::Test
    )
    add_test(NAME qtmaps_benchmark_tests COMMAND qtmaps_benchmark_tests)
    add_test(NAME qtmaps_benchmark_summary_tests
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/benchmark/test_summarize_benchmarks.py")
    add_test(NAME qtmaps_benchmark_cli_smoke
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/benchmark/smoke_cli.py"
            "$<TARGET_FILE:qtmaps>"
            "${CMAKE_SOURCE_DIR}")
    set_tests_properties(qtmaps_benchmark_cli_smoke PROPERTIES
        ENVIRONMENT "QT_QUICK_BACKEND=software;QTMAPS_ASSERT_SOFTWARE_BACKEND=1")

    add_executable(qtmaps_perf_tests tests/perf/tst_Performance.cpp)
    target_link_libraries(qtmaps_perf_tests PRIVATE qtmaps_perf Qt5::Test)
    add_test(NAME qtmaps_perf_tests COMMAND qtmaps_perf_tests)

    add_executable(qtmaps_diagnostics_tests tests/map/tst_CameraProbeController.cpp)
    target_link_libraries(qtmaps_diagnostics_tests PRIVATE qtmaps_diagnostics Qt5::Test)
    add_test(NAME qtmaps_diagnostics_tests COMMAND qtmaps_diagnostics_tests)

    add_executable(qtmaps_map_pipeline_tests tests/map/tst_MapPipeline.cpp)
    target_link_libraries(qtmaps_map_pipeline_tests PRIVATE
        qtmaps_diagnostics
        qtmaps_map
        Qt5::Network
        Qt5::Sql
        Qt5::Test
    )
    add_test(NAME qtmaps_map_pipeline_tests COMMAND qtmaps_map_pipeline_tests)
    add_test(NAME qtmaps_raster_package_tool_tests
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tests/map/test_build_hanoi_raster_package.py")

    add_executable(qtmaps_routing_tests tests/routing/tst_Routing.cpp)
    target_compile_definitions(qtmaps_routing_tests PRIVATE
        QTMAPS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    target_link_libraries(qtmaps_routing_tests PRIVATE qtmaps_routing Qt5::Test)
    add_test(NAME qtmaps_routing_tests COMMAND qtmaps_routing_tests)

    add_executable(qtmaps_route_navigation_tests
        tests/routing/tst_RouteNavigation.cpp
    )
    target_link_libraries(qtmaps_route_navigation_tests PRIVATE
        qtmaps_routing
        Qt5::Test
    )
    add_test(NAME qtmaps_route_navigation_tests COMMAND qtmaps_route_navigation_tests)

    add_executable(qtmaps_valhalla_runtime_tests
        tests/routing/tst_ValhallaRuntime.cpp
        ${QTMAPS_RESOURCES}
    )
    target_compile_definitions(qtmaps_valhalla_runtime_tests PRIVATE
        QTMAPS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
    target_link_libraries(qtmaps_valhalla_runtime_tests PRIVATE
        qtmaps_app_core
        qtmaps_perf
        qtmaps_routing
        Qt5::Network
        Qt5::Test
    )
    add_test(NAME qtmaps_valhalla_runtime_tests COMMAND qtmaps_valhalla_runtime_tests)

    add_executable(qtmaps_routing_qml_tests
        tests/routing/tst_RoutingQml.cpp
        ${QTMAPS_RESOURCES}
    )
    target_link_libraries(qtmaps_routing_qml_tests PRIVATE
        qtmaps_app_core
        qtmaps_map
        qtmaps_perf
        qtmaps_routing
        Qt5::Gui
        Qt5::Qml
        Qt5::Quick
        Qt5::Test
    )
    add_test(NAME qtmaps_routing_qml_tests COMMAND qtmaps_routing_qml_tests)
    set_tests_properties(qtmaps_routing_qml_tests PROPERTIES
        ENVIRONMENT
            "QT_QPA_PLATFORM=offscreen;QML2_IMPORT_PATH=${CMAKE_SOURCE_DIR}/generated/unit-01b/qml-runtime/usr/lib/x86_64-linux-gnu/qt5/qml;QT_QUICK_BACKEND=software")

endif()
