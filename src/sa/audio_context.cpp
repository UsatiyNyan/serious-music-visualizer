//
// Created by usatiynyan on 12/31/23.
//

#include "sa/audio_context.hpp"

#include <assert.hpp>
#include <spdlog/spdlog.h>

namespace sa {

namespace {

constexpr ma_format format = ma_format_f32;
constexpr ma_uint32 device_sample_rate = 48000;

ma_result log_error(ma_result result) {
    spdlog::error(ma::result_description(result));
    return result;
}

} // namespace

tl::expected<sl::meta::defer, ma_result> make_running_device_guard(const ma::device_uptr& device) {
    return ma::device_start(device).map([&](tl::monostate) {
        return sl::meta::defer{ [&] { ASSERT(ma::device_stop(device).map_error(log_error)); } };
    });
}

AudioContext::AudioContext(const std::vector<ma_backend>& backends, const ma_context_config& context_config)
    : context_{ *ASSERT(ma::context_init(backends, context_config).map_error(log_error)) } {
    auto [playback_infos, capture_infos] = *ASSERT(ma::context_get_devices(context_).map_error(log_error));
    playback_infos_ = playback_infos;
    capture_infos_ = capture_infos;
}

std::string_view AudioContext::backend_name() const { return ma::get_backend_name(context_->backend); }

std::span<ma_device_info> AudioContext::playback_infos() const { return playback_infos_; }

std::span<ma_device_info> AudioContext::capture_infos() const { return capture_infos_; }

tl::expected<ma::device_uptr, ma_result> AudioContext::create_device(
    ma_device_type device_type,
    AudioDeviceConfig playback_config,
    AudioDeviceConfig capture_config,
    ma_device_data_proc data_callback,
    void* user_data
) const {
    ma_device_config device_config = ma_device_config_init(device_type);
    if (device_type & ma_device_type::ma_device_type_playback) {
        ASSERT(playback_config.index < playback_infos_.size());
        device_config.playback.format = format;
        device_config.playback.channels = playback_config.channels;
        device_config.playback.pDeviceID = &playback_infos_[playback_config.index].id;
    }
    if (device_type & ma_device_type::ma_device_type_capture || device_type == ma_device_type_loopback) {
        ASSERT(capture_config.index < capture_infos_.size());
        device_config.capture.format = format;
        device_config.capture.channels = capture_config.channels;
        device_config.capture.pDeviceID = &capture_infos_[capture_config.index].id;
    }

    device_config.sampleRate = device_sample_rate;
    device_config.dataCallback = data_callback;
    device_config.pUserData = user_data;

    return ma::device_init(context_, device_config).map_error(log_error);
}

} // namespace sa
