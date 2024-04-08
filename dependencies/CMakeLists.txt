# CMake packages
cpmaddpackage(
        NAME serious-meta-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-meta-library.git"
        GIT_TAG main)
cpmaddpackage(
        NAME serious-graphics-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-graphics-library.git"
        GIT_TAG main)
cpmaddpackage(
        NAME serious-calculation-library
        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-calculation-library.git"
        GIT_TAG main)
# TODO
#cpmaddpackage(
#        NAME serious-concurrency-library
#        GIT_REPOSITORY "git@github.com:UsatiyNyan/serious-concurrency-library.git"
#        GIT_TAG main)
# Oh well...
cpmaddpackage(
        NAME SPSCQueue
        GIT_REPOSITORY "git@github.com:rigtorp/SPSCQueue.git"
        GIT_TAG v1.1)

cpmaddpackage(
        NAME fmt
        GIT_REPOSITORY "git@github.com:fmtlib/fmt.git"
        GIT_TAG 10.1.1)
cpmaddpackage(
        NAME libassert
        GIT_REPOSITORY "git@github.com:jeremy-rifkin/libassert.git"
        GIT_TAG main)

cpmaddpackage(
        NAME range-v3
        GIT_REPOSITORY "git@github.com:ericniebler/range-v3.git"
        GIT_TAG 0.12.0)


include("${CMAKE_CURRENT_SOURCE_DIR}/tl.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/spdlog.cmake")

# Manually compiled
add_subdirectory(miniaudio)

sl_target_link_system_libraries(${PROJECT_NAME}
        PUBLIC
        # fp components
        serious-meta-library
        range-v3::range-v3
        tl::optional
        tl::expected

        # fmt/log
        fmt::fmt
        spdlog::spdlog

        # utils
        libassert::assert

        # audio
        miniaudio

        # graphics
        serious-graphics-library

        # fft
        serious-calculation-library

        # concurrency
        SPSCQueue::SPSCQueue
)
