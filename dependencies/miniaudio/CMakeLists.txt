cpmaddpackage(
        NAME miniaudio
        DOWNLOAD_ONLY
        URL https://github.com/mackron/miniaudio/archive/refs/tags/0.11.23.tar.gz
)
add_library(miniaudio-wrapper STATIC src/miniaudio.cpp)
target_include_directories(miniaudio-wrapper
        PUBLIC
        include
        "${miniaudio_SOURCE_DIR}"
)
message("miniaudio_SOURCE_DIR=${miniaudio_SOURCE_DIR}")
sl_target_link_system_libraries(miniaudio-wrapper
        PUBLIC
        sl::meta
)
