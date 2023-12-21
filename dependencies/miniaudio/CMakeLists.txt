cpmaddpackage(
        NAME miniaudio
        DOWNLOAD_ONLY
        GIT_REPOSITORY "git@github.com:mackron/miniaudio.git"
        GIT_TAG master
)
add_library(miniaudio STATIC src/miniaudio.cpp)
target_include_directories(miniaudio
        PUBLIC include
        PRIVATE "${miniaudio_SOURCE_DIR}"
)
sl_target_link_system_libraries(miniaudio
        PRIVATE
        assert

        PUBLIC
        tl::optional
)
