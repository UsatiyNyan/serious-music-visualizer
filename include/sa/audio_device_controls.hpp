//
// Created by usatiynyan on 1/5/24.
//

#pragma once

#include <miniaudio/miniaudio.hpp>
#include <tl/optional.hpp>

namespace sa {

class AudioDeviceControls {
public:
    struct State {
        ma_device_type device_type;
        std::size_t index;

        bool operator<=>(const State& other) const = default;
    };

public:
    explicit AudioDeviceControls(std::span<const ma_device_info> capture_infos);
    tl::optional<State> update();

private:
    std::vector<std::string> capture_names_;
    tl::optional<ma_device_type> device_type_;
    tl::optional<std::size_t> index_;
};

} // namespace sa
