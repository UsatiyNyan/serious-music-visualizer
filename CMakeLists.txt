cmake_minimum_required(VERSION 3.21)

include(FetchContent)
FetchContent_Declare(
        serious-cmake-library
        GIT_REPOSITORY git@github.com:UsatiyNyan/serious-cmake-library.git
        GIT_TAG main
)
FetchContent_MakeAvailable(serious-cmake-library)

# Project

sl_project_prologue(
        CXX_STANDARD 20
        C_STANDARD 17)

project(
        serious-music-visualizer
        VERSION 0.0.1
        DESCRIPTION "music visualizer"
        HOMEPAGE_URL "TODO"
        LANGUAGES C CXX)

add_library(${PROJECT_NAME} STATIC
        src/sa/audio_context.cpp
        src/sa/audio_device_controls.cpp
        src/sa/audio_data.cpp
        src/sa/audio_data_debug_window.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)
sl_target_attach_directory(${PROJECT_NAME} shaders)

add_executable(${PROJECT_NAME}-app src/main.cpp)
target_link_libraries(${PROJECT_NAME}-app PRIVATE ${PROJECT_NAME})

add_subdirectory(dependencies)

# Tests and examples

if (NOT PROJECT_IS_TOP_LEVEL)
    return()
endif ()

sl_compiler_warnings(${PROJECT_NAME} WARNINGS_AS_ERRORS EXTRA_GCC_WARNINGS "-Wno-null-dereference")

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

add_subdirectory(examples)
