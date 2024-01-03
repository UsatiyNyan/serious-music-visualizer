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

        if (ImGui::Begin("device controls", nullptr, ImGuiWindowFlags_NoMove)) {
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

            ImGui::End();
        }

        return sl::meta::combine(device_type, index).map([](const auto& values) {
            return sl::meta::construct_from_tuple<AudioDeviceState>(values);
        });
    }
};

void draw_complex_data(
    std::string_view name,
    const auto values_getter,
    std::span<const std::complex<float>> points,
    float scale_min,
    float scale_max
) {
    if (points.empty()) {
        return;
    }

    constexpr ImVec2 graph_size(0, 80);
    void* points_data = const_cast<void*>(static_cast<const void*>(points.data()));
    ImGui::PlotLines(
        name.data(),
        values_getter,
        points_data,
        static_cast<int>(points.size()),
        0,
        nullptr,
        scale_min,
        scale_max,
        graph_size
    );
}

std::vector<std::complex<float>> to_log_domain(std::span<const std::complex<float>> linear_domain_points) {
    std::vector<std::complex<float>> log_domain_points(linear_domain_points.size());
    const float N = static_cast<float>(linear_domain_points.size());
    const float logN = std::log(N);

    // Preprocess the data
    for (std::size_t i = 0; i < linear_domain_points.size(); ++i) {
        // Compute the log-scaled x-coordinate
        const float logX = std::log(static_cast<float>(i + 1)) / logN;

        // Find the corresponding index (this may involve interpolation)
        const std::size_t index = static_cast<std::size_t>(logX * (N - 1));
        log_domain_points[index] = linear_domain_points[i];
    }

    return log_domain_points;
}

// purely for debugging purposes
std::vector<std::complex<float>> interpolate_log_domain(const std::vector<std::complex<float>>& log_domain_points) {
    std::vector<std::complex<float>> interpolated_points = log_domain_points;
    const std::size_t N_size = log_domain_points.size();

    for (std::size_t i = 0; i < N_size; ++i) {
        if (interpolated_points[i] == std::complex<float>(0, 0)) { // Check if the point is unfilled
            // Find the nearest non-zero points before and after index i
            std::size_t prev = i;
            std::size_t next = i;
            while (prev > 0 && interpolated_points[prev] == std::complex<float>(0, 0)) {
                --prev;
            }
            while (next < N_size - 1 && interpolated_points[next] == std::complex<float>(0, 0)) {
                ++next;
            }

            // Linear interpolation for complex numbers
            float alpha = static_cast<float>(i - prev) / static_cast<float>(next - prev);
            interpolated_points[i] = (1 - alpha) * interpolated_points[prev] + alpha * interpolated_points[next];
        }
    }

    return interpolated_points;
}

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

    constexpr ma_uint32 audio_capture_channels = 2;
    constexpr std::size_t audio_data_frame_count = 1024;
    constexpr std::size_t max_audio_data_coef = 16;
    std::vector<float> audio_data;
    audio_data.reserve(audio_capture_channels * audio_data_frame_count);
    std::vector<std::complex<float>> time_domain_input(audio_data_frame_count);

    while (!current_window.should_close()) {
        if (current_window.is_key_pressed(GLFW_KEY_ESCAPE)) {
            current_window.set_should_close(true);
        }
        current_window.clear(GL_COLOR_BUFFER_BIT);

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

        // FETCH AUDIO DATA
        if (const auto* audio_data_ptr = audio_data_callback->data_queue.front();
            audio_data_ptr != nullptr
            && audio_data.size() < max_audio_data_coef * audio_capture_channels * audio_data_frame_count) {
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
                audio_data.begin() + static_cast<std::int64_t>(audio_capture_channels * audio_data_frame_count)
            );
        }

        // DEBUG AUDIO DATA PROCESSING
        if (ImGui::Begin("debug audio data processing")) {
            const auto freq_domain_output =
                sl::calc::fourier::fft_recursive<sl::calc::fourier::direction::time_to_freq>(
                    std::span<const std::complex<float>>{ time_domain_input }
                );

            draw_complex_data(
                "time_domain",
                [](void* data, int idx) -> float {
                    const auto* v = static_cast<const std::complex<float>*>(data);
                    return v[static_cast<std::size_t>(idx)].real();
                },
                time_domain_input,
                -1.0f,
                1.0f
            );
            draw_complex_data(
                "freq_domain",
                [](void* data, int idx) -> float {
                    const auto* v = static_cast<const std::complex<float>*>(data);
                    return std::abs(v[static_cast<std::size_t>(idx)]);
                },
                freq_domain_output,
                0.0f,
                64.0f
            );

            const std::span half_freq_domain{
                freq_domain_output.begin(),
                freq_domain_output.begin() + static_cast<std::int64_t>(freq_domain_output.size() / 2),
            };
            draw_complex_data(
                "half_freq_domain",
                [](void* data, int idx) -> float {
                    const auto* v = static_cast<const std::complex<float>*>(data);
                    return std::abs(v[static_cast<std::size_t>(idx)]);
                },
                half_freq_domain,
                0.0f,
                64.0f
            );

            constexpr auto log_abs_value_getter = [](void* data, int idx) -> float {
                const auto* v = static_cast<const std::complex<float>*>(data);
                return std::log(std::abs(v[static_cast<std::size_t>(idx)]));
            };
            draw_complex_data("log_half_freq_domain", log_abs_value_getter, half_freq_domain, 0.0f, std::log(64.0f));

            const auto half_freq_log_domain = to_log_domain(half_freq_domain);
            draw_complex_data(
                "log_half_freq_log_domain", log_abs_value_getter, half_freq_log_domain, 0.0f, std::log(64.0f)
            );

            const auto interpolate_half_freq_log_domain = interpolate_log_domain(half_freq_log_domain);
            draw_complex_data(
                "log_interpolate_half_freq_log_domain",
                log_abs_value_getter,
                interpolate_half_freq_log_domain,
                0.0f,
                std::log(64.0f)
            );

            ImGui::End();
        }

        imgui_context.render();
        current_window.swap_buffers();
        ctx.poll_events();
    }

    return 0;
}
