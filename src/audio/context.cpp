//
// Created by usatiynyan.
//

#include "audio/context.hpp"

#include <miniaudio/miniaudio.hpp>
#include <sl/meta/assert.hpp>
#include <spdlog/spdlog.h>

namespace audio {

Context::Context(const std::vector<ma_backend>& backends, ma_context_config context_config)
    : context_{ *ASSERT_VAL(ma::context_init(backends, add_logging(context_config, log_))) } {
    auto [playback_infos, capture_infos] = *ASSERT_VAL(ma::context_get_devices(context_));
    playback_infos_ = playback_infos;
    capture_infos_ = capture_infos;
}

std::string_view Context::backend_name() const { return ma::get_backend_name(context_->backend); }

sl::meta::result<ma::device_uptr, ma_result> Context::create_device(
    ma_device_type device_type,
    const DataConfig& data_config,
    const DeviceConfig& playback_config,
    const DeviceConfig& capture_config,
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

    device_config.sampleRate = data_config.sample_rate;
    device_config.dataCallback = data_callback;
    device_config.pUserData = user_data;

    return ma::device_init(context_, device_config).map_error([&](ma_result result) {
        spdlog::error("[miniaudio] {}", ma::result_description(result));
        return result;
    });
}

ma_context_config Context::add_logging(ma_context_config context_config, ma_log& log) {
    const auto result = ma_log_register_callback(
        &log,
        ma_log_callback{
            .onLog =
                [](void* pUserData, ma_uint32 level, const char* pMessage) {
                    switch (level) {
                    case MA_LOG_LEVEL_ERROR:
                        spdlog::error("[miniaudio] {}", pMessage);
                        break;

                    case MA_LOG_LEVEL_WARNING:
                        spdlog::warn("[miniaudio] {}", pMessage);
                        break;

                    case MA_LOG_LEVEL_INFO:
                        spdlog::info("[miniaudio] {}", pMessage);
                        break;

                    case MA_LOG_LEVEL_DEBUG:
                        spdlog::debug("[miniaudio] {}", pMessage);
                        break;

                    default:
                        spdlog::trace("[miniaudio] {}", pMessage);
                        break;
                    }
                },
            .pUserData = nullptr,
        }
    );
    ASSERT(result == MA_SUCCESS);

    // context_config.pLog = &log;
    return context_config;
}

} // namespace audio
