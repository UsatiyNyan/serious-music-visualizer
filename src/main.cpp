//
// Created by usatiynyan on 12/14/23.
//

#include "sa/audio.hpp"
#include "sl/calc/fourier.hpp"
#include "sl/gfx.hpp"

#include <rigtorp/SPSCQueue.h>

#include <range/v3/to_container.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/stride.hpp>
#include <range/v3/view/take.hpp>

#include <assert.hpp>
#include <fmt/format.h>
#include <iostream>

namespace views = ranges::views;

struct AudioCallback {
    rigtorp::SPSCQueue<std::vector<float>> data_queue{ 1 };

    void operator()(std::span<const float> input) {
        [[maybe_unused]] const bool pushed = data_queue.try_emplace(input.begin(), input.end());
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
    const sl::gfx::Size2I window_size{ 800, 600 };
    const auto window = ASSERT(sl::gfx::Window::create(ctx, "serious music visualizer", window_size));
    window->FramebufferSize_cb = [&window](GLsizei width, GLsizei height) {
        sl::gfx::Window::Current{ *window }.viewport(sl::gfx::Vec2I{}, sl::gfx::Size2I{ width, height });
    };
    auto current_window =
        window->make_current(sl::gfx::Vec2I{}, window_size, sl::gfx::Color4F{ 0.2f, 0.3f, 0.3f, 1.0f });
    sl::gfx::ImGuiContext imgui_context{ ctx_options, *window };

    const sa::AudioContext audio_context;
    for (const auto& [i, capture_info] : views::enumerate(audio_context.capture_infos())) {
        fmt::println("capture[{}]={} ", i, capture_info.name);
    }
    const std::size_t capture_index = [] {
        fmt::print("choose capture index: ");
        std::size_t capture_index_ = 0;
        std::cin >> capture_index_;
        return capture_index_;
    }();

    const auto audio_callback = std::make_unique<AudioCallback>();
    const auto capture_device = *ASSERT(audio_context.create_capture_device(capture_index, audio_callback));
    const std::size_t capture_channels = capture_device->capture.channels;

    constexpr std::size_t audio_data_frame_count = 1024;
    const std::size_t audio_data_size = audio_data_frame_count * capture_channels;
    std::vector<float> audio_data;
    audio_data.reserve(audio_data_size);

    ASSERT(ma::device_start(capture_device));

    while (!current_window.should_close()) {
        if (current_window.is_key_pressed(GLFW_KEY_ESCAPE)) {
            current_window.set_should_close(true);
        }
        const auto* audio_data_ptr = audio_callback->data_queue.front();
        if (audio_data_ptr == nullptr) {
            continue;
        }
        audio_data.insert(audio_data.end(), audio_data_ptr->begin(), audio_data_ptr->end());
        audio_callback->data_queue.pop();

        while (audio_data.size() >= audio_data_size) {
            // capture.channels=2
            // take only 0th channel, which is left for stereo, for now
            const auto time_domain_input = audio_data //
                                           | views::stride(capture_channels) //
                                           | views::take(audio_data_frame_count) //
                                           | ranges::to<std::vector<std::complex<float>>>();
            audio_data.erase(audio_data.begin(), audio_data.begin() + static_cast<std::int64_t>(audio_data_size));

            const auto freq_domain_output =
                sl::calc::fourier::fft_recursive<sl::calc::fourier::direction::time_to_freq>(std::span{
                    time_domain_input });


            current_window.clear(GL_COLOR_BUFFER_BIT);
            imgui_context.new_frame();

            draw_complex_data(
                "time_domain",
                [](void* data, int idx) -> float {
                    const auto* v = static_cast<const std::complex<float>*>(data);
                    return v[static_cast<std::size_t>(idx)].real();
                },
                std::span{ time_domain_input },
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

            imgui_context.render();
            current_window.swap_buffers();
            ctx.poll_events();
        }
    }

    return 0;
}
