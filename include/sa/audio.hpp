//
// Created by usatiynyan on 12/31/23.
//

#pragma once

#include <miniaudio/miniaudio.hpp>

#include <tl/expected.hpp>

#include <memory>
#include <span>
#include <string_view>
#include <tuple>

namespace sa {

class AudioContext {
public:
    explicit AudioContext(
        const std::vector<ma_backend>& backends = { ma_backend_pulseaudio, ma_backend_wasapi },
        const ma_context_config& context_config = ma_context_config_init()
    );

    [[nodiscard]] std::string_view backend_name() const;
    [[nodiscard]] std::span<ma_device_info> playback_infos() const;
    [[nodiscard]] std::span<ma_device_info> capture_infos() const;

    template <typename Callable>
    [[nodiscard]] tl::expected<ma::device_uptr, ma_result>
        create_playback_device(std::size_t playback_index, const std::unique_ptr<Callable>& callable) const;

    template <typename Callable>
    [[nodiscard]] tl::expected<ma::device_uptr, ma_result>
        create_capture_device(std::size_t capture_index, const std::unique_ptr<Callable>& callable) const;

    template <typename Callable>
    [[nodiscard]] tl::expected<ma::device_uptr, ma_result> create_duplex_device(
        std::size_t playback_index,
        std::size_t capture_index,
        const std::unique_ptr<Callable>& callable
    ) const;

    template <typename Callable>
    [[nodiscard]] tl::expected<ma::device_uptr, ma_result>
        create_loopback_device(std::size_t capture_index, const std::unique_ptr<Callable>& callable) const;

private:
    [[nodiscard]] tl::expected<ma::device_uptr, ma_result> create_device(
        ma_device_type device_type,
        std::size_t playback_index,
        std::size_t capture_index,
        ma_device_data_proc data_callback,
        void* user_data
    ) const;

    static void validate(const ma_device* device);

private:
    ma::context_uptr context_;
    std::span<ma_device_info> playback_infos_;
    std::span<ma_device_info> capture_infos_;
};

} // namespace sm

#include "audio_inl.hpp"
