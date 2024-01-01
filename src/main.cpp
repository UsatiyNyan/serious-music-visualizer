//
// Created by usatiynyan on 12/14/23.
//

#include "sa/audio.hpp"

#include "sl/calc/fourier.hpp"

#include <rigtorp/SPSCQueue.h>

#include <range/v3/to_container.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/stride.hpp>
#include <range/v3/view/take.hpp>

#include <assert.hpp>
#include <fmt/format.h>
#include <iostream>

struct AudioCallback {
    rigtorp::SPSCQueue<std::vector<float>> data_queue{ 1 };

    void operator()(std::span<const float> input) {
        [[maybe_unused]] const bool pushed = data_queue.try_emplace(input.begin(), input.end());
    }
};


int main() {
    namespace views = ranges::views;

    const sa::AudioContext audio_context;

    for (const auto& [i, capture_info] : views::enumerate(audio_context.capture_infos())) {
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
            const auto time_domain_input = audio_data //
                                           | views::stride(capture_channels) //
                                           | views::take(audio_data_frame_count) //
                                           | ranges::to<std::vector<std::complex<float>>>();
            audio_data.erase(audio_data.begin(), audio_data.begin() + static_cast<std::int64_t>(audio_data_size));

            const auto freq_domain_output =
                sl::calc::fft<sl::calc::fourier::direction::time_to_freq>(std::span{ time_domain_input });

            // TODO: draw time_domain_input and freq_domain_output
        }
    }

    return 0;
}
