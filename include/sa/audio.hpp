//
// Created by usatiynyan on 12/31/23.
//

#pragma once

#include <miniaudio/miniaudio.hpp>

#include <sl/meta/lifetime/defer.hpp>

#include <tl/expected.hpp>

#include <memory>
#include <span>
#include <string_view>
#include <tuple>

namespace sa {

tl::expected<sl::defer, ma_result> make_running_device_guard(const ma::device_uptr& device);

struct AudioDeviceConfig {
    std::size_t index;
    ma_uint32 channels;
};

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
        create_playback_device(AudioDeviceConfig playback_config, const std::unique_ptr<Callable>& callable) const;

    template <typename Callable>
    [[nodiscard]] tl::expected<ma::device_uptr, ma_result>
        create_capture_device(AudioDeviceConfig capture_config, const std::unique_ptr<Callable>& callable) const;

    template <typename Callable>
    [[nodiscard]] tl::expected<ma::device_uptr, ma_result> create_duplex_device(
        AudioDeviceConfig playback_config,
        AudioDeviceConfig capture_config,
        const std::unique_ptr<Callable>& callable
    ) const;

    template <typename Callable>
    [[nodiscard]] tl::expected<ma::device_uptr, ma_result>
        create_loopback_device(AudioDeviceConfig capture_config, const std::unique_ptr<Callable>& callable) const;

private:
    [[nodiscard]] tl::expected<ma::device_uptr, ma_result> create_device(
        ma_device_type device_type,
        AudioDeviceConfig playback_config,
        AudioDeviceConfig capture_config,
        ma_device_data_proc data_callback,
        void* user_data
    ) const;

private:
    ma::context_uptr context_;
    std::span<ma_device_info> playback_infos_;
    std::span<ma_device_info> capture_infos_;
};

} // namespace sa

#include "audio_inl.hpp"
