//
// Created by usatiynyan.
//

#pragma once

#include "audio/data.hpp"
#include <miniaudio/miniaudio.hpp>

#include <sl/meta/lifetime/defer.hpp>
#include <sl/meta/monad/result.hpp>

namespace audio {

struct DeviceConfig {
    std::size_t index;
    ma_uint32 channels;
};

struct Context {
    static constexpr ma_format format = ma_format_f32;

public:
    explicit Context(
        const std::vector<ma_backend>& backends = { ma_backend_pulseaudio, ma_backend_coreaudio, ma_backend_wasapi },
        ma_context_config context_config = ma_context_config_init()
    );

    [[nodiscard]] std::string_view backend_name() const;
    [[nodiscard]] std::span<ma_device_info> playback_infos() const { return playback_infos_; }
    [[nodiscard]] std::span<ma_device_info> capture_infos() const { return capture_infos_; }

    template <typename Callable>
    [[nodiscard]] sl::meta::result<ma::device_uptr, ma_result> create_playback_device(
        const DataConfig& data_config,
        const DeviceConfig& playback_config,
        const std::unique_ptr<Callable>& callable
    ) const {
        constexpr auto data_callback =
            [](ma_device* device, void* output, [[maybe_unused]] const void* input, ma_uint32 frame_count) {
                (*static_cast<Callable*>(device->pUserData)) //
                    (std::span{ static_cast<float*>(output), frame_count * device->playback.channels });
            };

        return create_device(ma_device_type_playback, data_config, playback_config, {}, data_callback, callable.get());
    }

    template <typename Callable>
    [[nodiscard]] sl::meta::result<ma::device_uptr, ma_result> create_capture_device(
        const DataConfig& data_config,
        const DeviceConfig& capture_config,
        const std::unique_ptr<Callable>& callable
    ) const {
        constexpr auto data_callback =
            [](ma_device* device, [[maybe_unused]] void* output, const void* input, ma_uint32 frame_count) {
                (*static_cast<Callable*>(device->pUserData)) //
                    (std::span{ static_cast<const float*>(input), frame_count * device->capture.channels });
            };

        return create_device(ma_device_type_capture, data_config, {}, capture_config, data_callback, callable.get());
    }

    template <typename Callable>
    [[nodiscard]] sl::meta::result<ma::device_uptr, ma_result> create_duplex_device(
        const DataConfig& data_config,
        const DeviceConfig& playback_config,
        const DeviceConfig& capture_config,
        const std::unique_ptr<Callable>& callable
    ) const {
        constexpr auto data_callback = [](ma_device* device, void* output, const void* input, ma_uint32 frame_count) {
            (*static_cast<Callable*>(device->pUserData)) //
                (std::span{ static_cast<float*>(output), frame_count * device->playback.channels },
                 std::span{ static_cast<const float*>(input), frame_count * device->capture.channels });
        };

        return create_device(
            ma_device_type_duplex, data_config, playback_config, capture_config, data_callback, callable.get()
        );
    }

    template <typename Callable>
    [[nodiscard]] sl::meta::result<ma::device_uptr, ma_result> create_loopback_device(
        const DataConfig& data_config,
        const DeviceConfig& capture_config,
        const std::unique_ptr<Callable>& callable
    ) const {
        constexpr auto data_callback =
            [](ma_device* device, [[maybe_unused]] void* output, const void* input, ma_uint32 frame_count) {
                (*static_cast<Callable*>(device->pUserData)) //
                    (std::span{ static_cast<const float*>(input), frame_count * device->capture.channels });
            };

        return create_device(ma_device_type_loopback, data_config, {}, capture_config, data_callback, callable.get());
    }

private:
    [[nodiscard]] sl::meta::result<ma::device_uptr, ma_result> create_device(
        ma_device_type device_type,
        const DataConfig& data_config,
        const DeviceConfig& playback_config,
        const DeviceConfig& capture_config,
        ma_device_data_proc data_callback,
        void* user_data
    ) const;

    static ma_context_config add_logging(ma_context_config context_config, ma_log& log);

private:
    ma_log log_{};
    ma::context_uptr context_;
    std::span<ma_device_info> playback_infos_;
    std::span<ma_device_info> capture_infos_;
};

} // namespace audio
