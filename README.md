# serious-music-visualizer
By serious programmer.

# demo

- [log mode](https://www.youtube.com/watch?v=tjGEA5m6VYo)
- [ray marching](https://www.youtube.com/watch?v=y7plAgssw8g)

# build

## on Linux with X11

Dependencies:

```shell
libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

Build:

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release -Dserious-graphics-library_PROTOCOL=X11
cmake --build build --parallel 8 --target serious-music-visualizer
```

## on Linux with Wayland

Dependencies:

```shell
extra-cmake-modules wayland wayland-scanner wayland-protocols libxkbcommon libffi libglvnd
```

Build:

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release -Dserious-graphics-library_PROTOCOL=WAYLAND
cmake --build build --parallel 8 --target serious-music-visualizer
```

## on other platforms

Build:

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 8 --target serious-music-visualizer
```

# run

```shell
./build/serious-music-visualizer
```

And feel free to mess around in render settings.

> [!IMPORTANT]
> Visualizer won't start processing until you select `capture` or `loopback` device type and a capture source.
> Capture sources that have "Monitor ..." in their names are the ones that listen to desktop audio.

# TODO

use native amount of channels for capture, use capture.channels after init

