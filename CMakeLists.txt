cmake_minimum_required(VERSION 3.28)

if(CMAKE_HOST_APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Target architecture")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "Minimum macOS version")
endif()

project(jgrid VERSION 0.1.0 LANGUAGES C CXX)

if(MSVC)
    set(CMAKE_CXX_STANDARD 23)
else()
    set(CMAKE_CXX_STANDARD 26)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

option(JGRID_BUILD_TESTS "Build unit tests" ON)
option(JGRID_BUILD_DEMO "Build the demo app" ON)
option(JGRID_BUILD_EXAMPLES "Build the documentation examples" OFF)
option(JGRID_BUILD_MODULE_CHECK "Build a program that uses the Grid through juce_add_module" OFF)
option(JGRID_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(JGRID_CLANG_TIDY "Run clang-tidy while compiling" OFF)
set(JGRID_SANITIZE "" CACHE STRING "Sanitizers to enable: address, undefined, thread (comma separated)")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(JgridOptions)
include(JgridDependencies)

if(JGRID_CLANG_TIDY)
    find_program(JGRID_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
endif()

add_library(jgrid_static STATIC
    modules/jgrid/core/jgrid_Axis.cpp
    modules/jgrid/core/jgrid_Change.cpp
    modules/jgrid/core/jgrid_Clipboard.cpp
    modules/jgrid/core/jgrid_Editing.cpp
    modules/jgrid/core/jgrid_GridModelNotifier.cpp
    modules/jgrid/core/jgrid_HeaderLabels.cpp
    modules/jgrid/core/jgrid_Navigation.cpp
    modules/jgrid/core/jgrid_RowOrder.cpp
    modules/jgrid/core/jgrid_Selection.cpp
    modules/jgrid/core/jgrid_Sorting.cpp
    modules/jgrid/core/jgrid_ViewModel.cpp
    modules/jgrid/grid/jgrid_Grid.cpp
    modules/jgrid/grid/jgrid_GridAccessibility.cpp
    modules/jgrid/grid/jgrid_GridClipboard.cpp
    modules/jgrid/grid/jgrid_GridEditing.cpp
    modules/jgrid/grid/jgrid_GridLayout.cpp
    modules/jgrid/grid/jgrid_GridMenus.cpp
    modules/jgrid/grid/jgrid_GridRows.cpp
    modules/jgrid/grid/jgrid_GridSelection.cpp
    modules/jgrid/grid/jgrid_GridSorting.cpp)
add_library(jgrid::jgrid ALIAS jgrid_static)
target_include_directories(jgrid_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/modules")
target_link_libraries(jgrid_static PUBLIC jgrid::juce)
jgrid_configure_target(jgrid_static)

if(JGRID_BUILD_DEMO)
    add_subdirectory(demo)
endif()

if(JGRID_BUILD_TESTS OR JGRID_BUILD_MODULE_CHECK OR JGRID_BUILD_EXAMPLES)
    enable_testing()
endif()

if(JGRID_BUILD_EXAMPLES)
    add_subdirectory(docs/examples)
endif()

if(JGRID_BUILD_MODULE_CHECK)
    add_subdirectory(tests/module)
endif()

if(JGRID_BUILD_TESTS)
    add_subdirectory(tests)
endif()
