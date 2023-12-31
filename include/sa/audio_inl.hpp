//
// Created by usatiynyan on 12/31/23.
//

#pragma once

namespace sa {

template <typename Callable>
tl::expected<ma::device_uptr, ma_result>
    AudioContext::create_playback_device(std::size_t playback_index, const std::unique_ptr<Callable>& callable) const {
    constexpr auto data_callback =
        [](ma_device* device, void* output, [[maybe_unused]] const void* input, ma_uint32 frame_count) {
            validate(device);
            (*static_cast<Callable*>(device->pUserData)) //
                (std::cref(*device),
                 std::span{ static_cast<float*>(output), frame_count * device->playback.channels },
                 frame_count);
        };

    return create_device(ma_device_type_playback, playback_index, 0, data_callback, callable.get());
}

template <typename Callable>
tl::expected<ma::device_uptr, ma_result>
    AudioContext::create_capture_device(std::size_t capture_index, const std::unique_ptr<Callable>& callable) const {
    constexpr auto data_callback =
        [](ma_device* device, [[maybe_unused]] void* output, const void* input, ma_uint32 frame_count) {
            validate(device);
            (*static_cast<Callable*>(device->pUserData)) //
                (std::cref(*device),
                 std::span{ static_cast<const float*>(input), frame_count * device->capture.channels },
                 frame_count);
        };

    return create_device(ma_device_type_capture, 0, capture_index, data_callback, callable.get());
}

template <typename Callable>
tl::expected<ma::device_uptr, ma_result> AudioContext::create_duplex_device(
    std::size_t playback_index,
    std::size_t capture_index,
    const std::unique_ptr<Callable>& callable
) const {
    constexpr auto data_callback = [](ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
        validate(device);
        (*static_cast<Callable*>(device->pUserData)) //
            (std::cref(*device),
             std::span{ static_cast<float*>(output), frame_count * device->playback.channels },
             std::span{ static_cast<const float*>(input), frame_count * device->capture.channels },
             frame_count);
    };

    return create_device(ma_device_type_duplex, playback_index, capture_index, data_callback, callable.get());
}

template <typename Callable>
tl::expected<ma::device_uptr, ma_result>
    AudioContext::create_loopback_device(std::size_t capture_index, const std::unique_ptr<Callable>& callable) const {
    constexpr auto data_callback =
        [](ma_device* device, [[maybe_unused]] void* output, const void* input, ma_uint32 frame_count) {
            validate(device);
            (*static_cast<Callable*>(device->pUserData)) //
                (std::cref(*device),
                 std::span{ static_cast<const float*>(input), frame_count * device->capture.channels },
                 frame_count);
        };

    return create_device(ma_device_type_loopback, 0, capture_index, data_callback, callable.get());
}

} // namespace sa
