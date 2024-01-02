//
// Created by usatiynyan on 12/31/23.
//

#pragma once

namespace sa {

template <typename Callable>
tl::expected<ma::device_uptr, ma_result>
    AudioContext::create_playback_device(AudioDeviceConfig playback_config, const std::unique_ptr<Callable>& callable)
        const {
    constexpr auto data_callback =
        [](ma_device* device, void* output, [[maybe_unused]] const void* input, ma_uint32 frame_count) {
            (*static_cast<Callable*>(device->pUserData)) //
                (std::span{ static_cast<float*>(output), frame_count * device->playback.channels });
        };

    return create_device(ma_device_type_playback, playback_config, {}, data_callback, callable.get());
}

template <typename Callable>
tl::expected<ma::device_uptr, ma_result>
    AudioContext::create_capture_device(AudioDeviceConfig capture_config, const std::unique_ptr<Callable>& callable)
        const {
    constexpr auto data_callback =
        [](ma_device* device, [[maybe_unused]] void* output, const void* input, ma_uint32 frame_count) {
            (*static_cast<Callable*>(device->pUserData)) //
                (std::span{ static_cast<const float*>(input), frame_count * device->capture.channels });
        };

    return create_device(ma_device_type_capture, {}, capture_config, data_callback, callable.get());
}

template <typename Callable>
tl::expected<ma::device_uptr, ma_result> AudioContext::create_duplex_device(
    AudioDeviceConfig playback_config,
    AudioDeviceConfig capture_config,
    const std::unique_ptr<Callable>& callable
) const {
    constexpr auto data_callback = [](ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
        (*static_cast<Callable*>(device->pUserData)) //
            (std::span{ static_cast<float*>(output), frame_count * device->playback.channels },
             std::span{ static_cast<const float*>(input), frame_count * device->capture.channels });
    };

    return create_device(ma_device_type_duplex, playback_config, capture_config, data_callback, callable.get());
}

template <typename Callable>
tl::expected<ma::device_uptr, ma_result>
    AudioContext::create_loopback_device(AudioDeviceConfig capture_config, const std::unique_ptr<Callable>& callable)
        const {
    constexpr auto data_callback =
        [](ma_device* device, [[maybe_unused]] void* output, const void* input, ma_uint32 frame_count) {
            (*static_cast<Callable*>(device->pUserData)) //
                (std::span{ static_cast<const float*>(input), frame_count * device->capture.channels });
        };

    return create_device(ma_device_type_loopback, {}, capture_config, data_callback, callable.get());
}

} // namespace sa
