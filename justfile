generate BUILD_TYPE="Debug" PROTOCOL="WAYLAND":
    cmake -S . -B ./build -DCMAKE_BUILD_TYPE={{BUILD_TYPE}} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -GNinja --log-level=TRACE -Dserious-graphics-library_PROTOCOL={{PROTOCOL}}

build: generate
    cmake --build ./build --parallel $(nproc) --target serious-music-visualizer
