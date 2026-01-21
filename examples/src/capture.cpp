//
// Created by usatiynyan on 12/27/23.
//

#include "miniaudio/miniaudio.hpp"

#include <fmt/format.h>
#include <sl/meta/assert.hpp>

#include <atomic>
#include <chrono>
#include <thread>

int main() {
    const ma_context_config context_config = ma_context_config_init();
    const auto context = *ASSERT_VAL(ma::context_init({}, context_config));
    fmt::println("context backend: {}", ma_get_backend_name(context->backend));

    constexpr auto data_callback =
        [](ma_device* device, [[maybe_unused]] void* output, const void* input, ma_uint32 frame_count) {
            ASSERT_VAL(device->capture.format == ma_format_f32);
            const float* input_f32 = static_cast<const float*>(input);

            float curr_highest_amplitude = 0;
            for (std::uint32_t frame_index = 0; frame_index < frame_count; ++frame_index) {
                for (std::uint32_t channel_index = 0; channel_index < device->capture.channels; ++channel_index) {
                    const float amplitude = input_f32[frame_index * device->capture.channels + channel_index];
                    curr_highest_amplitude = std::max(amplitude, curr_highest_amplitude);
                }
            }

            auto* highest_amplitude = ASSERT_VAL(static_cast<std::atomic<std::float_t>*>(device->pUserData));
            highest_amplitude->store(curr_highest_amplitude, std::memory_order::release);
        };

    constexpr ma_uint32 device_channels = 2;
    constexpr ma_uint32 device_sample_rate = 48000;
    const auto highest_amplitude = std::make_unique<std::atomic<std::float_t>>();

    ma_device_config device_config = ma_device_config_init(ma_device_type_capture);
    device_config.capture.format = ma_format_f32;
    device_config.capture.channels = device_channels;
    device_config.sampleRate = device_sample_rate;
    device_config.dataCallback = data_callback;
    device_config.pUserData = highest_amplitude.get();

    const auto device = *ASSERT_VAL(ma::device_init(context, device_config));
    fmt::println("Device Name: {}", device->playback.name);

    ASSERT(ma::device_start(device));

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds{ 33 });
        const float curr_highest_amplitude = highest_amplitude->load(std::memory_order::acquire);
        const std::size_t stars_count = static_cast<std::size_t>(curr_highest_amplitude * 256.0f);
        for (std::size_t i = 0; i < stars_count; ++i) {
            fmt::print("*");
        }
        fmt::println("|");
    }

    return 0;
}
