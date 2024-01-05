//
// Created by usatiynyan on 1/5/24.
//

#pragma once

#include "sa/audio_data.hpp"

namespace sa {

void show_audio_data_debug_window(const AudioDataConfig& config, std::span<const float> processed_freq_domain_output);

} // namespace sa
