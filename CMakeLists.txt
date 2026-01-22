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
        CXX_STANDARD 23
        C_STANDARD 17)

project(
        serious-music-visualizer
        VERSION 2.0.2
        DESCRIPTION "music visualizer"
        HOMEPAGE_URL "TODO"
        LANGUAGES C CXX)

add_library(${PROJECT_NAME}-lib STATIC
    src/audio/context.cpp
    src/audio/data.cpp
    src/visualizer/audio.cpp
    src/visualizer/scene.cpp
    src/visualizer/render.cpp
)
target_include_directories(${PROJECT_NAME}-lib PUBLIC include)
sl_target_attach_directory(${PROJECT_NAME}-lib shaders)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}-lib)

add_subdirectory(dependencies)

# Tests and examples

if (NOT PROJECT_IS_TOP_LEVEL)
    return()
endif ()

sl_compiler_warnings(${PROJECT_NAME}-lib WARNINGS_AS_ERRORS)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

add_subdirectory(examples)
