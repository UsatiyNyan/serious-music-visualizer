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

## on Windows

You are highly encouraged to generate with `Ninja` and build with `clang`.

Build:

```shell
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_COMPILER=clang
cmake --build build --parallel 8 --target serious-music-visualizer
```

## on macOS

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
> Capture sources that have "Monitor ..." in their names are the ones that listen to desktop audio (Linux only).

## macOS: Desktop Audio Capture

macOS does not support native loopback audio capture. To capture desktop audio, you need to install a virtual audio driver like [BlackHole](https://github.com/ExistentialAudio/BlackHole).

### Setup

1. Install BlackHole:
   ```shell
   brew install blackhole-2ch
   ```

2. Create a Multi-Output Device:
   - Open **Audio MIDI Setup** (search in Spotlight)
   - Click **+** in the bottom left → **Create Multi-Output Device**
   - Check both your speakers/headphones AND **BlackHole 2ch**
   - Right-click the Multi-Output Device → **Use This Device For Sound Output**

3. In the visualizer, select **capture** device type and choose **BlackHole 2ch** as the capture source.

> [!NOTE]
> The `loopback` device type only works on Linux (PulseAudio) and Windows (WASAPI).
> On macOS, use `capture` with BlackHole instead.

# TODO

- [ ] use native amount of channels for capture, use capture.channels after init

