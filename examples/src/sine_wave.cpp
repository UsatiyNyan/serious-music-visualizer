//
// Created by usatiynyan on 12/26/23.
//

#include "miniaudio/miniaudio.hpp"

#include <assert.hpp>
#include <fmt/format.h>

int main() {
    const ma_context_config context_config = ma_context_config_init();
    const auto context = *ASSERT(ma::context_init({}, context_config));

    constexpr auto data_callback =
        [](ma_device* device, void* output, [[maybe_unused]] const void* input, ma_uint32 frame_count) {
            auto* sine_wave = ASSERT(static_cast<ma_waveform*>(device->pUserData));
            ma_waveform_read_pcm_frames(sine_wave, output, frame_count, nullptr);
        };

    constexpr ma_uint32 device_channels = 2;
    constexpr ma_uint32 device_sample_rate = 48000;
    const auto sine_wave = std::make_unique<ma_waveform>();

    ma_device_config device_config = ma_device_config_init(ma_device_type_playback);
    device_config.playback.format = ma_format_f32;
    device_config.playback.channels = device_channels;
    device_config.sampleRate = device_sample_rate;
    device_config.dataCallback = data_callback;
    device_config.pUserData = sine_wave.get();

    const auto device = *ASSERT(ma::device_init(context, device_config));
    fmt::println("device name: {}", device->playback.name);

    ma_waveform_config sineWaveConfig = ma_waveform_config_init(
        device->playback.format, device->playback.channels, device->sampleRate, ma_waveform_type_sine, 0.2, 220
    );
    ma_waveform_init(&sineWaveConfig, sine_wave.get());

    ASSERT(ma::device_start(device));

    fmt::println("press enter to quit...");
    std::getchar();

    return 0;
}
