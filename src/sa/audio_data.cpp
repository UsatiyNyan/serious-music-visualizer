//
// Created by usatiynyan on 1/5/24.
//

#include "sa/audio_data.hpp"

namespace sa {

void AudioDataCallback::operator()(std::span<const float> input) {
    [[maybe_unused]] const bool pushed = data_queue_.try_emplace(input.begin(), input.end());
}

bool AudioDataCallback::fetch(std::vector<float>& output) {
    const auto* data_ptr = data_queue_.front();
    if (data_ptr == nullptr) {
        return false;
    }
    output.insert(output.end(), data_ptr->begin(), data_ptr->end());
    data_queue_.pop();
    return true;
}

std::tuple<std::vector<float>, std::vector<std::complex<float>>> //
create_audio_data_state(const AudioDataConfig& config) {
    std::vector<float> audio_data;
    audio_data.reserve(config.frame_max_size());
    std::vector<std::complex<float>> time_domain_input(config.frame_count);
    return { std::move(audio_data), std::move(time_domain_input) };
}
} // namespace sa
