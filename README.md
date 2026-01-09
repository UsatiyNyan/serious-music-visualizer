# serious-music-visualizer
By serious programmer.

# demo

- [log mode](https://www.youtube.com/watch?v=tjGEA5m6VYo)
- [ray marching](https://www.youtube.com/watch?v=y7plAgssw8g)

# dependencies

## X11

Example for debian-based systems:

```shell
sudo apt install \
  libx11-dev \
  libxrandr-dev \
  libxinerama-dev \
  libxcursor-dev \
  libxi-dev
```

# build

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 8 --target serious-music-visualizer
```

# TODO

use native amount of channels for capture, use capture.channels after init

