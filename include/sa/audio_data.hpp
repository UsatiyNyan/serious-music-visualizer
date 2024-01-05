//
// Created by usatiynyan on 1/5/24.
//

#pragma once

#include <rigtorp/SPSCQueue.h>
#include <miniaudio/miniaudio.hpp>

#include <complex>
#include <span>
#include <tuple>
#include <vector>

namespace sa {

struct AudioDataConfig {
    ma_uint32 capture_channels;
    std::size_t frame_count;
    std::size_t max_frame_count;
    std::size_t frame_window;

    constexpr std::size_t frame_size() const { return capture_channels * frame_count; }
    constexpr std::size_t frame_max_size() const { return capture_channels * max_frame_count; }
    constexpr std::int64_t frame_window_size() const { return static_cast<std::int64_t>(capture_channels * frame_window); }
};

class AudioDataCallback {
public:
    explicit AudioDataCallback(std::size_t queue_capacity) : data_queue_{ queue_capacity } {}

    void operator()(std::span<const float> input);
    bool fetch(std::vector<float>& output);

private:
    rigtorp::SPSCQueue<std::vector<float>> data_queue_;
};

std::tuple<std::vector<float>, std::vector<std::complex<float>>> create_audio_data_state(const AudioDataConfig& config);

} // namespace sa
