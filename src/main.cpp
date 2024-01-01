//
// Created by usatiynyan on 12/14/23.
//

#include "sa/audio.hpp"

#include "sl/calc/fourier.hpp"

#include <assert.hpp>
#include <fmt/format.h>
#include <range/v3/view/enumerate.hpp>
#include <rigtorp/SPSCQueue.h>

#include <iostream>

struct AudioCallback {
    rigtorp::SPSCQueue<std::vector<float>> data_queue{ 1 };

    void operator()(std::span<const float> input) {
        [[maybe_unused]] const bool pushed = data_queue.try_emplace(input.begin(), input.end());
    }
};


int main() {
    const sa::AudioContext audio_context;

    for (const auto& [i, capture_info] : ranges::views::enumerate(audio_context.capture_infos())) {
        fmt::println("capture[{}]={} ", i, capture_info.name);
    }

    const std::size_t capture_index = [] {
        fmt::print("choose capture index: ");
        std::size_t capture_index_ = 0;
        std::cin >> capture_index_;
        return capture_index_;
    }();

    const auto audio_callback = std::make_unique<AudioCallback>();
    const auto capture_device = *ASSERT(audio_context.create_capture_device(capture_index, audio_callback));
    const std::size_t capture_channels = capture_device->capture.channels;

    constexpr std::size_t audio_data_frame_count = 1024;
    const std::size_t audio_data_size = audio_data_frame_count * capture_channels;
    std::vector<float> audio_data;
    audio_data.reserve(audio_data_size);

    ASSERT(ma::device_start(capture_device));

    while (true) {
        const auto* audio_data_ptr = audio_callback->data_queue.front();
        if (audio_data_ptr == nullptr) {
            continue;
        }
        audio_data.insert(audio_data.end(), audio_data_ptr->begin(), audio_data_ptr->end());
        audio_callback->data_queue.pop();

        while (audio_data.size() >= audio_data_size) {
            // capture.channels=2
            // take only 0th channel, which is left for stereo, for now
            const auto time_domain_input = [&audio_data, capture_channels] {
                std::vector<std::complex<float>> time_domain_input_(audio_data_frame_count);
                for (std::size_t i = 0; i < audio_data_frame_count; ++i) {
                    time_domain_input_[i] = audio_data[i * capture_channels];
                }
                return time_domain_input_;
            }();

            audio_data.erase(audio_data.begin(), audio_data.begin() + static_cast<std::int64_t>(audio_data_size));

            const auto freq_domain_output =
                sl::calc::fft<sl::calc::fourier::direction::time_to_freq>(std::span{ time_domain_input });

            const auto max_amplitude_it = std::max_element(
                freq_domain_output.begin(),
                freq_domain_output.end(),
                [](const std::complex<float> l, const std::complex<float> r) { return std::abs(l) < std::abs(r); }
            );
            if (max_amplitude_it == freq_domain_output.end()) {
                continue;
            }

            fmt::print("{:<8}", audio_data.size());

            const auto stars_count = static_cast<std::size_t>(std::abs(*max_amplitude_it));
            for (std::size_t i = 0; i < stars_count; ++i) {
                fmt::print("*");
            }
            fmt::println("|");
        }
    }

    return 0;
}
