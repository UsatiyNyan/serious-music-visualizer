//
// Created by usatiynyan.
//

#pragma once

#include <concepts>
#include <miniaudio/miniaudio.hpp>

#include <sl/exec/model/executor.hpp>

#include <span>
#include <vector>

namespace audio {

struct DataConfig {
    constexpr DataConfig(
        ma_uint32 capture_channels,
        ma_uint32 sample_rate,
        std::size_t frame_count,
        std::size_t max_frame_count,
        std::size_t frame_window
    )
        : capture_channels{ capture_channels }, //
          sample_rate{ sample_rate }, //
          frame_count{ frame_count }, //
          max_frame_count{ max_frame_count }, //
          frame_window{ frame_window }, //
          frame_size{ capture_channels * frame_count }, //
          frame_max_size{ capture_channels * max_frame_count }, //
          frame_window_size{ static_cast<std::int64_t>(capture_channels * frame_window) } {}

public:
    ma_uint32 capture_channels;
    ma_uint32 sample_rate;
    std::size_t frame_count;
    std::size_t max_frame_count;
    std::size_t frame_window;
    std::size_t frame_size;
    std::size_t frame_max_size;
    std::int64_t frame_window_size;
};

struct DataCallback {
    using ChunkT = std::vector<float>;

public:
    constexpr explicit DataCallback(const DataConfig& config, sl::exec::executor& sync_executor)
        : sync_executor_{ sync_executor } {
        chunk_.reserve(config.frame_max_size);
    }

    void operator()(std::span<const float> input);

    [[nodiscard]] bool try_consume(const DataConfig& config, std::invocable<std::span<const float>> auto callable) {
        if (config.frame_size > chunk_.size()) {
            return false;
        }

        const auto begin = chunk_.begin();
        const auto end = std::next(begin, config.frame_size);
        callable(std::span<const float>{ begin, end });
        chunk_.erase(begin, end);

        return true;
    }

private:
    ChunkT chunk_;
    sl::exec::executor& sync_executor_;
};

} // namespace audio
