# CMake packages
cpmaddpackage(
        NAME serious-game-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-game-library.git"
        GIT_TAG 0.0.0)
cpmaddpackage(
        NAME serious-calculation-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-calculation-library.git"
        GIT_TAG 0.0.0)

# Manually compiled
add_subdirectory(miniaudio)

sl_target_link_system_libraries(${PROJECT_NAME}
        PUBLIC
        # serious libraries
        sl::game
        sl::calc

        # audio
        miniaudio-wrapper
)
