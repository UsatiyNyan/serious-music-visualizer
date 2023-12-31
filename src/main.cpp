//
// Created by usatiynyan on 12/14/23.
//

#include "sa/audio.hpp"

#include <assert.hpp>
#include <fmt/format.h>
#include <range/v3/view/enumerate.hpp>
#include <rigtorp/SPSCQueue.h>

#include <iostream>

using AudioData = std::vector<float>;

struct AudioCallback {
    rigtorp::SPSCQueue<AudioData> data_queue{1};

    void operator()(
        [[maybe_unused]] const ma_device& device,
        std::span<const float> input,
        [[maybe_unused]] ma_uint32 frame_count
    ) {
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

    ASSERT(ma::device_start(capture_device));

    while (true) {
        auto* audio_data_ptr = audio_callback->data_queue.front();
        if (audio_data_ptr == nullptr) {
            continue;
        }
        const auto audio_data = std::move(*audio_data_ptr);
        audio_callback->data_queue.pop();

        const auto max_amplitude_it = std::max_element(audio_data.begin(), audio_data.end());
        if (max_amplitude_it == audio_data.end()) {
            continue;
        }

        const auto stars_count = static_cast<std::size_t>(*max_amplitude_it * 256.0f);
        for (std::size_t i = 0; i < stars_count; ++i) {
            fmt::print("*");
        }
        fmt::println("|");
    }

    return 0;
}
