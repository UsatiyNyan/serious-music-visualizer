//
// Created by usatiynyan on 12/14/23.
//

#include "sa/audio.hpp"

#include <sl/calc/fourier.hpp>
#include <sl/gfx.hpp>
#include <sl/meta/optional/combine.hpp>
#include <sl/meta/tuple/construct_from_tuple.hpp>

#include <rigtorp/SPSCQueue.h>

#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/stride.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>

#include <tl/optional.hpp>

#include <assert.hpp>
#include <fmt/format.h>

struct AudioDataCallback {
    rigtorp::SPSCQueue<std::vector<float>> data_queue;

    void operator()(std::span<const float> input) {
        [[maybe_unused]] const bool pushed = data_queue.try_emplace(input.begin(), input.end());
    }
};

struct AudioDeviceState {
    ma_device_type device_type;
    std::size_t index;

    bool operator<=>(const AudioDeviceState& other) const = default;
};

struct AudioDeviceControls {
    std::vector<std::string> audio_capture_names;
    tl::optional<ma_device_type> device_type;
    tl::optional<std::size_t> index;

    tl::optional<AudioDeviceState> update() {
        constexpr std::array device_type_to_name_map{
            std::pair{ ma_device_type_capture, std::string_view{ "capture" } },
            std::pair{ ma_device_type_loopback, std::string_view{ "loopback" } },
        };

        if (ImGui::Begin("device controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            ImGui::SetWindowPos(ImVec2{ 0.0f, 0.0f });
            ImGui::SetWindowSize(ImVec2{ 0.0f, 80.0f });

            const auto preview_device_type =
                device_type //
                    .map([device_type_to_name_map](auto device_type_) {
                        const auto it = std::find_if(
                            device_type_to_name_map.begin(),
                            device_type_to_name_map.end(),
                            [device_type_](const auto& device_type_to_name) {
                                return device_type_to_name.first == device_type_;
                            }
                        );
                        return it == device_type_to_name_map.end() ? std::string_view{} : it->second;
                    })
                    .value_or(std::string_view{});

            if (ImGui::BeginCombo("device type", preview_device_type.data())) {
                for (const auto& [a_device_type, a_device_type_name] : device_type_to_name_map) {
                    if (ImGui::Selectable(a_device_type_name.data())) {
                        device_type = a_device_type;
                    }
                }
                ImGui::EndCombo();
            }

            const auto preview_capture_source =
                index //
                    .map([this](auto index_) { return std::string_view{ audio_capture_names[index_] }; }) //
                    .value_or(std::string_view{});

            if (ImGui::BeginCombo("capture source", preview_capture_source.data())) {
                for (const auto& [i, audio_capture_name] : ranges::views::enumerate(audio_capture_names)) {
                    if (ImGui::Selectable(audio_capture_name.c_str())) {
                        index = i;
                    }
                }
                ImGui::EndCombo();
            }
        }
        ImGui::End();

        return sl::meta::combine(device_type, index).map([](const auto& values) {
            return sl::meta::construct_from_tuple<AudioDeviceState>(values);
        });
    }
};

int main() {
    const sl::gfx::Context::Options ctx_options{ 4, 6, GLFW_OPENGL_CORE_PROFILE };
    auto ctx = *ASSERT(sl::gfx::Context::create(ctx_options));
    const sl::gfx::Size2I window_size{ 1280, 720 };
    const auto window = ASSERT(sl::gfx::Window::create(ctx, "serious music visualizer", window_size));
    window->FramebufferSize_cb = [&window](GLsizei width, GLsizei height) {
        sl::gfx::Window::Current{ *window }.viewport(sl::gfx::Vec2I{}, sl::gfx::Size2I{ width, height });
    };
    auto current_window =
        window->make_current(sl::gfx::Vec2I{}, window_size, sl::gfx::Color4F{ 0.0f, 0.0f, 0.0f, 1.0f });
    sl::gfx::ImGuiContext imgui_context{ ctx_options, *window };
    sl::gfx::ImPlotContext implot_context{ imgui_context };

    const sa::AudioContext audio_context;
    tl::optional<AudioDeviceState> audio_device_state;
    AudioDeviceControls audio_device_controls{
        .audio_capture_names = ranges::views::enumerate(audio_context.capture_infos())
                               | ranges::views::transform([](const auto& i_and_device_info) {
                                     const auto& [i, device_info] = i_and_device_info;
                                     return fmt::format("capture[{}]={} ", i, device_info.name);
                                 })
                               | ranges::to<std::vector>(),
    };
    tl::expected<ma::device_uptr, ma_result> audio_device = tl::make_unexpected(MA_SUCCESS);
    tl::expected<sl::defer, ma_result> running_audio_device = tl::make_unexpected(MA_SUCCESS);

    constexpr std::size_t audio_data_queue_capacity = 1;
    const std::unique_ptr<AudioDataCallback> audio_data_callback{ new AudioDataCallback{
        .data_queue = rigtorp::SPSCQueue<std::vector<float>>{ audio_data_queue_capacity },
    } };

    constexpr ma_uint32 audio_capture_channels = 1;
    constexpr std::size_t audio_data_frame_count = 4096;
    constexpr std::size_t max_audio_data_frame_count = audio_data_frame_count * 16;
    constexpr std::size_t audio_data_frame_window = audio_data_frame_count / 2;
    std::vector<float> audio_data;
    audio_data.reserve(audio_capture_channels * audio_data_frame_count);
    std::vector<std::complex<float>> time_domain_input(audio_data_frame_count);

    while (!current_window.should_close()) {
        if (current_window.is_key_pressed(GLFW_KEY_ESCAPE)) {
            current_window.set_should_close(true);
        }
        current_window.clear(GL_COLOR_BUFFER_BIT);

        // DATA PROCESSING LAYER
        // FETCH AUDIO DATA
        if (const auto* audio_data_ptr = audio_data_callback->data_queue.front();
            audio_data_ptr != nullptr && audio_data.size() < audio_capture_channels * max_audio_data_frame_count) {
            audio_data.insert(audio_data.end(), audio_data_ptr->begin(), audio_data_ptr->end());
            audio_data_callback->data_queue.pop();
        }

        // FILL AUDIO DATA
        if (audio_data.size() >= audio_capture_channels * audio_data_frame_count) {
            // capture.channels=2
            // take only 0th channel, which is left for stereo, for now
            const auto time_domain_input_range = audio_data //
                                                 | ranges::views::stride(audio_capture_channels) //
                                                 | ranges::views::take(audio_data_frame_count);
            time_domain_input.assign(time_domain_input_range.begin(), time_domain_input_range.end());

            audio_data.erase(
                audio_data.begin(),
                audio_data.begin() + static_cast<std::int64_t>(audio_capture_channels * audio_data_frame_window)
            );
        }

        // CALCULATE FFT
        const auto freq_domain_output = sl::calc::fourier::fft<sl::calc::fourier::direction::time_to_freq>(
            std::span<const std::complex<float>>{ time_domain_input }
        );

        // DRAW LAYER
        // VISUALIZE AUDIO DATA
        {
            // TODO(@usatiynyan)
        }

        // IMGUI LAYER
        // TODO(@usatiynyan): test, check what if placed underneath draw layer
        imgui_context.new_frame();

        // DEVICE CONTROLS
        const auto new_audio_device_state = audio_device_controls.update();

        // INITIALIZE DEVICE
        if (new_audio_device_state && audio_device_state != new_audio_device_state) {
            audio_device_state = new_audio_device_state;

            // stop device if it is running
            running_audio_device = tl::make_unexpected(MA_SUCCESS);

            switch (audio_device_state->device_type) {
            case ma_device_type_capture:
                audio_device = audio_context.create_capture_device(
                    { .index = audio_device_state->index, .channels = audio_capture_channels }, audio_data_callback
                );
                break;
            case ma_device_type_loopback:
                audio_device = audio_context.create_loopback_device(
                    { .index = audio_device_state->index, .channels = audio_capture_channels }, audio_data_callback
                );
                break;
            default:
                audio_device = tl::make_unexpected(MA_DEVICE_TYPE_NOT_SUPPORTED);
                break;
            }

            running_audio_device = audio_device.and_then(sa::make_running_device_guard);
        }

        // DEBUG AUDIO DATA PROCESSING
        if (ImGui::Begin("debug audio data processing", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            constexpr ImVec2 debug_window_size{ 600.0f, 400.0f };
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const ImVec2 debug_window_pos{ viewport->WorkPos.x + viewport->WorkSize.x - debug_window_size.x,
                                           viewport->WorkPos.y };
            ImGui::SetWindowSize(debug_window_size);
            ImGui::SetWindowPos(debug_window_pos);

            ImGui::Text("FPS: %.1f", static_cast<double>(ImGui::GetIO().Framerate));

            if (ImPlot::BeginPlot("log domain", ImVec2{ -1.0f, -1.0f })) {
                constexpr double max_amp = static_cast<double>(audio_data_frame_count);
                const double log10_max_amp = std::log10(max_amp);
                const auto half_freq_domain = freq_domain_output //
                                              | ranges::views::take(freq_domain_output.size() / 2)
                                              | ranges::views::transform([](std::complex<float> complex_value) {
                                                    return std::log10(std::abs(complex_value));
                                                })
                                              | ranges::to<std::vector>();

                ImPlot::SetupAxisScale(ImAxis_X1, ImPlotScale_Log10);
                ImPlot::SetupAxisLimits(ImAxis_X1, 1.0, static_cast<double>(half_freq_domain.size()));
                ImPlot::SetupAxisLimits(ImAxis_Y1, -log10_max_amp, log10_max_amp);
                ImPlot::PlotLine(
                    "map (log10 abs) (F omega)", half_freq_domain.data(), static_cast<int>(half_freq_domain.size())
                );
                ImPlot::EndPlot();
            }
        }
        ImGui::End();

        imgui_context.render();
        current_window.swap_buffers();
        ctx.poll_events();
    }

    return 0;
}
