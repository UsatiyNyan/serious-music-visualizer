//
// Created by usatiynyan on 1/5/24.
//

#include "sa/audio_data_debug_window.hpp"

#include <imgui.h>
#include <implot.h>
#include <sl/meta/lifetime/defer.hpp>

namespace sa {

void show_audio_data_debug_window(const AudioDataConfig& config, std::span<const float> processed_freq_domain_output) {
    if (const sl::meta::defer imgui_end{ ImGui::End };
        ImGui::Begin("debug audio data processing", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        constexpr ImVec2 debug_window_size{ 600.0f, 400.0f };
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImVec2 debug_window_pos{ viewport->WorkPos.x + viewport->WorkSize.x - debug_window_size.x,
                                       viewport->WorkPos.y };
        ImGui::SetWindowSize(debug_window_size);
        ImGui::SetWindowPos(debug_window_pos);

        ImGui::Text("FPS: %.1f", static_cast<double>(ImGui::GetIO().Framerate));

        if (ImPlot::BeginPlot("log domain", ImVec2{ -1.0f, -1.0f })) {
            const double log10_max_amp = std::log10(static_cast<double>(config.frame_count));

            ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
            ImPlot::SetupAxisLimits(ImAxis_X1, 1.0, static_cast<double>(processed_freq_domain_output.size()));
            ImPlot::SetupAxisLimits(ImAxis_Y1, -log10_max_amp, log10_max_amp);
            ImPlot::PlotLine(
                "map (log10 abs) (F omega)",
                processed_freq_domain_output.data(),
                static_cast<int>(processed_freq_domain_output.size())
            );
            ImPlot::EndPlot();
        }
    }
}

} // namespace sa
