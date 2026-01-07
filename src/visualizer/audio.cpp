//
// Created by usatiynyan.
//

#include "visualizer/audio.hpp"
#include "visualizer/render.hpp"

#include <miniaudio/miniaudio.hpp>

#include <sl/calc/fourier.hpp>
#include <sl/meta/lifetime/defer.hpp>

#include <range/v3/view/stride.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>

#include <imgui.h>
#include <implot.h>
#include <sl/meta/match/match.hpp>
#include <sl/meta/match/match_map.hpp>

namespace visualizer {

sl::exec::async<entt::entity> create_audio_entity(
    sl::game::engine_context& e_ctx,
    sl::ecs::layer& layer,
    const audio::DataConfig& config,
    entt::entity render_entity
) {
    const auto entity = layer.registry.create();

    const auto& audio_state = layer.registry.emplace<AudioState>(
        entity,
        AudioState{
            .context{},
            .callback = std::make_unique<audio::DataCallback>(config, *e_ctx.sync_exec),
            .intermediate{},
            .device{
                .handle = sl::meta::err(MA_SUCCESS),
                .running = sl::meta::err(MA_SUCCESS),
            },
            .device_controls{
                .type{},
                .index{},
            },
        }
    );
    spdlog::info("selected backend={}", audio_state.context.backend_name());
    layer.registry.emplace<sl::game::update>(
        entity,
        [&config, render_entity](sl::ecs::layer& layer, entt::entity entity, sl::game::time_point) {
            auto& audio_state = layer.registry.get<AudioState>(entity);
            audio_update_process(config, layer, render_entity, audio_state);
            audio_update_device(config, audio_state);
        }
    );
    layer.registry.emplace<sl::game::overlay>(
        entity,
        [&config](sl::ecs::layer& layer, sl::gfx::imgui_frame& imgui_frame, entt::entity entity) {
            audio_overlay(config, layer, imgui_frame, entity);
        }
    );

    co_return entity;
}

void audio_update_process(
    const audio::DataConfig& config,
    sl::ecs::layer& layer,
    entt::entity render_entity,
    AudioState& audio_state
) {
    namespace r = ranges;
    namespace rv = r::views;

    // FETCH TIME DOMAIN INPUT
    while (audio_state.callback->try_consume(config, [&](std::span<const float> input) {
        ASSERT(input.size() == config.frame_size);
        const auto range = input | rv::stride(config.capture_channels);
        audio_state.intermediate.time_domain.assign(range.begin(), range.end());
    })) {}

    if (audio_state.intermediate.time_domain.size() != config.frame_count) {
        return;
    }

    // CALCULATE FFT (TIME DOMAIN -> FREQ DOMAIN)
    audio_state.intermediate.freq_domain = sl::calc::fft<sl::calc::fourier::direction::time_to_freq>(
        std::span<const std::complex<float>>{ audio_state.intermediate.time_domain }
    );

    audio_state.intermediate.half_freq_domain = audio_state.intermediate.freq_domain
                                                | rv::take(audio_state.intermediate.freq_domain.size() / 2)
                                                | r::to<std::vector>();

    audio_state.intermediate.abs_half_freq_domain =
        audio_state.intermediate.half_freq_domain //
        | rv::transform([](std::complex<float> x) { return std::abs(x); }) //
        | r::to<std::vector>();

    audio_state.intermediate.log_abs_half_freq_domain = audio_state.intermediate.abs_half_freq_domain //
                                                        | rv::transform([](float x) { return std::log(x); }) //
                                                        | r::to<std::vector>();

    if (auto* render_state = layer.registry.try_get<RenderState>(render_entity)) {
        const auto normalize_by = std::log(static_cast<float>(audio_state.intermediate.freq_domain.size()));
        auto normalized_output = audio_state.intermediate.log_abs_half_freq_domain
                                 | rv::transform([normalize_by](float value) { return value / normalize_by; })
                                 | r::to<std::vector>();
        render_state->normalized_freq_proc_output.set(std::move(normalized_output));
    }
}

void audio_update_device(const audio::DataConfig& config, AudioState& audio_state) {
    const sl::meta::maybe<ma_device_type> maybe_new_type = audio_state.device_controls.type.release();
    const sl::meta::maybe<std::size_t> maybe_new_index = audio_state.device_controls.index.release();
    if (!maybe_new_type.has_value() && !maybe_new_index.has_value()) { // no updates
        return;
    }

    const auto new_type = maybe_new_type.or_else([&] { return audio_state.device_controls.type.get(); });
    const auto new_index = maybe_new_index.or_else([&] { return audio_state.device_controls.index.get(); });
    if (!new_type.has_value() || !new_index.has_value()) { // not enough data
        return;
    }

    const audio::DeviceConfig device_config{
        .index = new_index.value(),
        .channels = config.capture_channels,
    };

    // stop running device
    audio_state.device.running = sl::meta::err(MA_SUCCESS);

    switch (new_type.value()) {
    case ma_device_type_capture:
        audio_state.device.handle =
            audio_state.context.create_capture_device(config, device_config, audio_state.callback);
        break;
    case ma_device_type_loopback:
        audio_state.device.handle =
            audio_state.context.create_loopback_device(config, device_config, audio_state.callback);
        break;
    default:
        audio_state.device.handle = sl::meta::err(MA_DEVICE_TYPE_NOT_SUPPORTED);
        break;
    }

    audio_state.device.running = audio_state.device.handle.and_then([](const ma::device_uptr& device) {
        return ma::device_start(device).map([&](sl::meta::unit) {
            return sl::meta::defer<>{ [&] { ASSERT(ma::device_stop(device)); } };
        });
    });
}

void audio_overlay(
    const audio::DataConfig& config,
    sl::ecs::layer& layer,
    sl::gfx::imgui_frame& imgui_frame,
    entt::entity audio_entity
) {
    constexpr auto device_type_to_name = [](ma_device_type device_type) -> std::string_view {
        switch (device_type) {
        case ma_device_type_playback:
            return "playback";
        case ma_device_type_capture:
            return "capture";
        case ma_device_type_duplex:
            return "duplex";
        case ma_device_type_loopback:
            return "loopback";
        default:
            break;
        }
        return "unknown";
    };
    constexpr std::array device_types{
        ma_device_type_capture,
        ma_device_type_loopback,
    };

    auto& audio_state = layer.registry.get<AudioState>(audio_entity);
    auto& context = audio_state.context;
    auto& device_controls = audio_state.device_controls;

    if (auto imgui_window = imgui_frame.begin( //
            "device controls"
            /* TODO: ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize */
        )) {
        ImGui::SetWindowPos(ImVec2{ 0.0f, 0.0f });
        ImGui::SetWindowSize(ImVec2{ 0.0f, 80.0f });

        const auto preview_device_type =
            device_controls.type.get().map(device_type_to_name).value_or(std::string_view{});
        if (ImGui::BeginCombo("device type", preview_device_type.data())) {
            for (const auto device_type : device_types) {
                if (ImGui::Selectable(device_type_to_name(device_type).data())) {
                    device_controls.type.set_if_ne(device_type);
                    device_controls.index = sl::meta::dirty<std::size_t>{}; // reset
                }
            }
            ImGui::EndCombo();
        }

        const auto preview_capture_source = device_controls.index.get()
                                                .map([&](std::size_t index) {
                                                    const auto& capture_info = context.capture_infos()[index];
                                                    return capture_info.name;
                                                })
                                                .value_or("");
        if (ImGui::BeginCombo("capture source", preview_capture_source)) {
            for (const auto& [index, capture_info] : ranges::views::enumerate(context.capture_infos())) {
                if (ImGui::Selectable(capture_info.name)) {
                    device_controls.index.set_if_ne(index);
                }
            }
            ImGui::EndCombo();
        }
    }

    if (auto imgui_window = imgui_frame.begin( //
            "debug audio data processing"
            /* TODO: ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize */
        )) {
        constexpr ImVec2 debug_window_size{ 600.0f, 400.0f };
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 debug_window_pos{ viewport->WorkPos.x + viewport->WorkSize.x - debug_window_size.x,
                                       viewport->WorkPos.y };
        ImGui::SetWindowSize(debug_window_size);
        ImGui::SetWindowPos(debug_window_pos);

        ImGui::Text("FPS: %.1f", static_cast<double>(ImGui::GetIO().Framerate));

        if (ImPlot::BeginPlot("log_abs_half_freq_domain", ImVec2{ -1.0f, 300.0f })) {
            const auto& vec = audio_state.intermediate.log_abs_half_freq_domain;
            const double log_max_amp = std::log(static_cast<double>(vec.size()));
            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
            ImPlot::SetupAxisLimits(ImAxis_X1, 1.0, static_cast<double>(vec.size()), ImPlotCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -log_max_amp, log_max_amp, ImPlotCond_Always);
            ImPlot::PlotLine("map (log abs) (F omega)", vec.data(), static_cast<int>(vec.size()));
            ImPlot::EndPlot();
        }
    }
}

} // namespace visualizer
