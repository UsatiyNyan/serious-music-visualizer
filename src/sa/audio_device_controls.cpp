//
// Created by usatiynyan on 1/5/24.
//

#include "sa/audio_device_controls.hpp"

#include <imgui.h>

#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/transform.hpp>

#include <sl/meta/lifetime/defer.hpp>
#include <sl/meta/optional/combine.hpp>
#include <sl/meta/tuple/construct_from_tuple.hpp>

namespace sa {

AudioDeviceControls::AudioDeviceControls(std::span<const ma_device_info> capture_infos)
    : capture_names_{ capture_infos | ranges::views::transform([](const auto& device_info) {
                          return std::string{ device_info.name };
                      })
                      | ranges::to<std::vector>() } {}

tl::optional<AudioDeviceControls::State> AudioDeviceControls::update() {
    constexpr std::array device_type_to_name_map{
        std::pair{ ma_device_type_capture, std::string_view{ "capture" } },
        std::pair{ ma_device_type_loopback, std::string_view{ "loopback" } },
    };

    if (const sl::defer imgui_end{ ImGui::End };
        ImGui::Begin("device controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
        ImGui::SetWindowPos(ImVec2{ 0.0f, 0.0f });
        ImGui::SetWindowSize(ImVec2{ 0.0f, 80.0f });

        const auto preview_device_type =
            device_type_ //
                .map([device_type_to_name_map](auto device_type) {
                    const auto it = std::find_if(
                        device_type_to_name_map.begin(),
                        device_type_to_name_map.end(),
                        [device_type](const auto& device_type_to_name) {
                            return device_type_to_name.first == device_type;
                        }
                    );
                    return it == device_type_to_name_map.end() ? std::string_view{} : it->second;
                })
                .value_or(std::string_view{});

        if (ImGui::BeginCombo("device type", preview_device_type.data())) {
            for (const auto& [a_device_type, a_device_type_name] : device_type_to_name_map) {
                if (ImGui::Selectable(a_device_type_name.data())) {
                    device_type_ = a_device_type;
                }
            }
            ImGui::EndCombo();
        }

        const auto preview_capture_source =
            index_ //
                .map([this](auto index) { return std::string_view{ capture_names_[index] }; }) //
                .value_or(std::string_view{});

        if (ImGui::BeginCombo("capture source", preview_capture_source.data())) {
            for (const auto& [i, audio_capture_name] : ranges::views::enumerate(capture_names_)) {
                if (ImGui::Selectable(audio_capture_name.c_str())) {
                    index_ = i;
                }
            }
            ImGui::EndCombo();
        }
    }

    return sl::meta::combine(device_type_, index_).map([](const auto& values) {
        return sl::meta::construct_from_tuple<State>(values);
    });
}
} // namespace sa
