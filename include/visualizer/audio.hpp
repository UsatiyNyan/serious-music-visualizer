//
// Created by usatiynyan.
//

#pragma once

#include "audio/context.hpp"
#include "audio/data.hpp"

#include <sl/game.hpp>
#include <sl/gfx.hpp>

#include <complex>

namespace visualizer {

struct AudioState {
    audio::Context context;
    std::unique_ptr<audio::DataCallback> callback;

    struct Intermediate {
        // TODO(@usatiynyan): time_domain_input for multiple channels
        std::vector<std::complex<float>> time_domain;
        std::vector<std::complex<float>> freq_domain;
        std::vector<std::complex<float>> half_freq_domain;
        std::vector<float> abs_half_freq_domain;
        std::vector<float> log_abs_half_freq_domain;
        std::vector<float> normalized_freq_domain_output;
        float sound_level = 0.0f;
    } intermediate;

    struct Device {
        sl::meta::result<ma::device_uptr, ma_result> handle;
        sl::meta::result<sl::meta::defer<>, ma_result> running;
    } device;

    struct DeviceControls {
        sl::meta::dirty<ma_device_type> type;
        sl::meta::dirty<std::size_t> index;
    } device_controls;
};

sl::exec::async<entt::entity> create_audio_entity(
    sl::game::engine_context& e_ctx,
    sl::ecs::layer& layer,
    const audio::DataConfig& config,
    entt::entity render_entity
);

void audio_update_process(
    const audio::DataConfig& config,
    sl::ecs::layer& layer,
    entt::entity render_entity,
    AudioState& audio_state,
    sl::game::time_point time_point
);

void audio_update_device(const audio::DataConfig& config, AudioState& audio_state);

void audio_overlay(const audio::DataConfig& config, sl::ecs::layer& layer, sl::gfx::imgui_frame&, entt::entity entity);

} // namespace visualizer
