//
// Created by usatiynyan on 12/14/23.
//

#include "sa/audio_context.hpp"
#include "sa/audio_data.hpp"
#include "sa/audio_data_debug_window.hpp"
#include "sa/audio_device_controls.hpp"

#include <sl/calc/fourier.hpp>
#include <sl/gfx.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <range/v3/view/stride.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>

#include <assert.hpp>

enum class DrawModes : unsigned {
    DRAW_RADIUS = 0,
    DEFAULT_FILL,
};

auto create_shader_program() {
    const std::array<sl::gfx::Shader, 2> shaders{
        *ASSERT(sl::gfx::Shader::load_from_file(sl::gfx::ShaderType::VERTEX, "shaders/music_visualizer.vert")),
        *ASSERT(sl::gfx::Shader::load_from_file(sl::gfx::ShaderType::FRAGMENT, "shaders/music_visualizer.frag")),
    };
    sl::gfx::ShaderProgram sp{ std::span{ shaders } };
    auto sp_bind = sp.bind();
    auto set_transform = *ASSERT(sp_bind.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));
    auto set_mode = *ASSERT(sp_bind.make_uniform_setter(glUniform1ui, "u_mode"));
    auto set_window_size = *ASSERT(sp_bind.make_uniform_setter(glUniform2f, "u_window_size"));
    return std::make_tuple(std::move(sp), std::move(set_transform), std::move(set_mode), std::move(set_window_size));
}

using SSBO = sl::gfx::Buffer<float, sl::gfx::BufferType::SHADER_STORAGE, sl::gfx::BufferUsage::DYNAMIC_DRAW>;

template <std::size_t size_>
SSBO create_ssbo() {
    SSBO ssbo;
    auto ssbo_bind = ssbo.bind();
    ssbo_bind.base<0>();
    ssbo_bind.initialize_data<size_>();
    return ssbo;
}

template <std::size_t size_>
void write_normalized_data_to_ssbo(SSBO& ssbo, std::span<const float, size_> data, float normalize_by) {
    auto mapped_ssbo = ssbo.bind().map<sl::gfx::BufferAccess::WRITE_ONLY, size_>();
    auto mapped_ssbo_data = mapped_ssbo.data();
    const auto normalize = [normalize_by](float value) { return value / normalize_by; };
    auto normalized_data = data | ranges::views::transform(normalize);
    std::copy(normalized_data.begin(), normalized_data.end(), mapped_ssbo_data.begin());
}

struct Vert {
    sl::gfx::va_attrib_field<2, float> pos;
};

auto create_buffers(std::span<const Vert, 4> vertices) {
    sl::gfx::VertexArrayBuilder va_builder;
    va_builder.attributes_from<Vert>();
    auto vb = va_builder.buffer<sl::gfx::BufferType::ARRAY, sl::gfx::BufferUsage::STATIC_DRAW>(vertices);
    constexpr std::array<unsigned, 6> indices{
        0u, 1u, 3u, // first triangle
        1u, 2u, 3u, // second triangle
    };
    auto eb =
        va_builder.buffer<sl::gfx::BufferType::ELEMENT_ARRAY, sl::gfx::BufferUsage::STATIC_DRAW>(std::span{ indices });
    auto va = std::move(va_builder).submit();
    return std::make_tuple(std::move(vb), std::move(eb), std::move(va));
}

int main() {
    const sl::gfx::Context::Options ctx_options{ 4, 6, GLFW_OPENGL_CORE_PROFILE };
    auto ctx = *ASSERT(sl::gfx::Context::create(ctx_options));
    const sl::gfx::Size2I window_size{ 1280, 720 };
    const auto window = ASSERT(sl::gfx::Window::create(ctx, "serious music visualizer", window_size));
    auto current_window =
        window->make_current(sl::gfx::Vec2I{}, window_size, sl::gfx::Color4F{ 0.0f, 0.0f, 0.0f, 1.0f });
    sl::gfx::ImGuiContext imgui_context{ ctx_options, *window };
    sl::gfx::ImPlotContext implot_context{ imgui_context };

    constexpr sa::AudioDataConfig audio_config{
        .capture_channels = 1,
        .frame_count = 1024 * 2,
        .max_frame_count = 1024 * 16,
        .frame_window = 1024,
    };

    constexpr std::array vertices{
        Vert{ +1.0f, +1.0f },
        Vert{ +1.0f, -1.0f },
        Vert{ -1.0f, -1.0f },
        Vert{ -1.0f, +1.0f },
    };
    auto [sp, set_transform, set_mode, set_window_size] = create_shader_program();
    auto [vb, eb, va] = create_buffers(vertices);
    auto ssbo = create_ssbo<audio_config.frame_count / 2>();

    // INIT
    {
        const auto sp_bind = sp.bind();
        const auto transform = glm::mat4(1.0f);
        set_transform(sp_bind, glm::value_ptr(transform));
        set_mode(sp.bind(), static_cast<unsigned>(DrawModes::DRAW_RADIUS));
        set_window_size(sp.bind(), static_cast<float>(window_size.width), static_cast<float>(window_size.height));

        window->FramebufferSize_cb = [&window,
                                      sp_ref = std::ref(sp),
                                      set_window_size_ref = std::ref(set_window_size)](GLsizei width, GLsizei height) {
            sl::gfx::Window::Current{ *window }.viewport(sl::gfx::Vec2I{}, sl::gfx::Size2I{ width, height });
            set_window_size_ref(sp_ref.get().bind(), static_cast<float>(width), static_cast<float>(height));
        };
    }

    const sa::AudioContext audio_context;
    tl::optional<sa::AudioDeviceControls::State> audio_device_controls_state;
    sa::AudioDeviceControls audio_device_controls{ audio_context.capture_infos() };
    tl::expected<ma::device_uptr, ma_result> audio_device = tl::make_unexpected(MA_SUCCESS);
    tl::expected<sl::meta::defer, ma_result> running_audio_device = tl::make_unexpected(MA_SUCCESS);

    constexpr std::size_t audio_data_queue_capacity = 1;
    const auto audio_data_callback = std::make_unique<sa::AudioDataCallback>(audio_data_queue_capacity);

    // TODO(@usatiynyan): time_domain_input for multiple channels
    auto [audio_data, time_domain_input] = sa::create_audio_data_state(audio_config);
    const auto time_domain_input_producer = //
        ranges::views::stride(audio_config.capture_channels) //
        | ranges::views::take(audio_config.frame_count);
    const auto freq_domain_output_processor =
        ranges::views::take(audio_config.frame_count / 2)
        | ranges::views::transform([](std::complex<float> x) { return std::log(std::abs(x)); })
        | ranges::to<std::vector>();

    while (!current_window.should_close()) {
        if (current_window.is_key_pressed(GLFW_KEY_ESCAPE)) {
            current_window.set_should_close(true);
        }
        current_window.clear(GL_COLOR_BUFFER_BIT);

        // DATA PROCESSING LAYER
        // FETCH AUDIO DATA
        if (audio_data.size() < audio_config.frame_max_size()) {
            [[maybe_unused]] const bool fetched = audio_data_callback->fetch(audio_data);
        }

        // FETCH TIME DOMAIN INPUT
        if (audio_data.size() >= audio_config.frame_size()) {
            const auto time_domain_input_range = time_domain_input_producer(audio_data);
            time_domain_input.assign(time_domain_input_range.begin(), time_domain_input_range.end());
        }

        // SLIDE AUDIO BUFFER WINDOW
        if (audio_data.size() >= audio_config.frame_size()) {
            audio_data.erase(audio_data.begin(), audio_data.begin() + audio_config.frame_window_size());
        }

        // CALCULATE FFT (TIME DOMAIN -> FREQ DOMAIN)
        const auto freq_domain_output = sl::calc::fft<sl::calc::fourier::direction::time_to_freq>(
            std::span<const std::complex<float>>{ time_domain_input }
        );
        const auto processed_freq_domain_output = freq_domain_output_processor(freq_domain_output);

        // DRAW LAYER
        // VISUALIZE AUDIO DATA
        {
            sl::gfx::Draw draw{ sp, va, {} };
            write_normalized_data_to_ssbo(
                ssbo,
                std::span<const float, audio_config.frame_count / 2>{ processed_freq_domain_output },
                std::log(static_cast<float>(audio_config.frame_count))
            );
            draw.elements(eb);
        }

        // IMGUI LAYER
        // TODO(@usatiynyan): test, check what if placed underneath draw layer
        imgui_context.new_frame();

        // DEVICE CONTROLS
        if (const auto new_audio_device_state = audio_device_controls.update();
            new_audio_device_state && audio_device_controls_state != new_audio_device_state) {
            audio_device_controls_state = new_audio_device_state;

            // stop device if it is running
            running_audio_device = tl::make_unexpected(MA_SUCCESS);

            const sa::AudioDeviceConfig audio_device_config{
                .index = audio_device_controls_state->index,
                .channels = audio_config.capture_channels,
            };

            switch (audio_device_controls_state->device_type) {
            case ma_device_type_capture:
                audio_device = audio_context.create_capture_device(audio_device_config, audio_data_callback);
                break;
            case ma_device_type_loopback:
                audio_device = audio_context.create_loopback_device(audio_device_config, audio_data_callback);
                break;
            default:
                audio_device = tl::make_unexpected(MA_DEVICE_TYPE_NOT_SUPPORTED);
                break;
            }

            running_audio_device = audio_device.and_then(sa::make_running_device_guard);
        }

        // DEBUG AUDIO DATA
        sa::show_audio_data_debug_window(audio_config, processed_freq_domain_output);

        imgui_context.render();
        current_window.swap_buffers();
        ctx.poll_events();
    }

    return 0;
}
