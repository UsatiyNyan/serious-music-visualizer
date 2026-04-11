# CMake packages
cpmaddpackage(
        NAME serious-graphics-library
        GIT_REPOSITORY "https://github.com/UsatiyNyan/serious-graphics-library.git"
        GIT_TAG 0.2.2
        GIT_SHALLOW TRUE
)
cpmaddpackage(
        NAME serious-game-library
        GIT_REPOSITORY "https://github.com/UsatiyNyan/serious-game-library.git"
        GIT_TAG 0.1.3
        GIT_SHALLOW TRUE
)
cpmaddpackage(
        NAME serious-calculation-library
        GIT_REPOSITORY "https://github.com/UsatiyNyan/serious-calculation-library.git"
        GIT_TAG 0.0.2
        GIT_SHALLOW TRUE
)

# Manually compiled
add_subdirectory(miniaudio)

sl_target_link_system_libraries(${PROJECT_NAME}-lib
        PUBLIC
        # serious libraries
        sl::game
        sl::calc

        # audio
        miniaudio-wrapper
)

if (UNIX AND NOT APPLE)
    sl_target_link_system_libraries(${PROJECT_NAME}-lib PRIVATE pulse)
endif()
