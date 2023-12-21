# CMake packages
cpmaddpackage(
        NAME serious-meta-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-meta-library.git"
        GIT_TAG main)
cpmaddpackage(
        NAME serious-graphics-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-graphics-library.git"
        GIT_TAG main)
# TODO
#cpmaddpackage(
#        NAME serious-calc-library
#        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-calc-library.git"
#        GIT_TAG main)
# TODO
#cpmaddpackage(
#        NAME serious-concurrency-library
#        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-concurrency-library.git"
#        GIT_TAG main)

cpmaddpackage(
        NAME fmt
        GIT_REPOSITORY "git@github.com:fmtlib/fmt.git"
        GIT_TAG 10.1.1)
cpmaddpackage(
        NAME spdlog
        GIT_REPOSITORY "git@github.com:gabime/spdlog.git"
        GIT_TAG v1.12.0)

cpmaddpackage(
        NAME libassert
        GIT_REPOSITORY "git@github.com:jeremy-rifkin/libassert.git"
        GIT_TAG main)

include("${CMAKE_CURRENT_SOURCE_DIR}/tl.cmake")

# Manually compiled
add_subdirectory(miniaudio)

sl_target_link_system_libraries(${PROJECT_NAME}
        # fp components
        serious-meta-library

        # fmt/log
        fmt::fmt
        spdlog::spdlog

        # utils
        assert

        # audio
        miniaudio

        # graphics
        serious-graphics-library
)