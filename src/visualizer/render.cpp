//
// Created by usatiynyan.
//

#include "visualizer/render.hpp"

#include <sl/game/graphics/buffer.hpp>
#include <sl/meta/enum/to_string.hpp>

namespace visualizer {

sl::exec::async<sl::game::shader>
    create_flat_shader(sl::game::engine_context& e_ctx, std::size_t ssbo_size, entt::entity render_entity) {
    const std::array<sl::gfx::shader, 2> shaders{
        *ASSERT_VAL( //
            sl::gfx::shader::load_from_file(sl::gfx::shader_type::vertex, e_ctx.root_path / "shaders/flat.vert")
        ),
        *ASSERT_VAL( //
            sl::gfx::shader::load_from_file(sl::gfx::shader_type::fragment, e_ctx.root_path / "shaders/flat.frag")
        ),
    };

    auto sp = *ASSERT_VAL(sl::gfx::shader_program::build(std::span{ shaders }));
    auto bound_sp = sp.bind();
    auto set_transform [[maybe_unused]] =
        *ASSERT_VAL(bound_sp.make_uniform_matrix_v_setter(glUniformMatrix4fv, "u_transform", 1, false));
    auto set_mode = *ASSERT_VAL(bound_sp.make_uniform_setter(glUniform1ui, "u_mode"));
    auto set_time = *ASSERT_VAL(bound_sp.make_uniform_setter(glUniform1f, "u_time"));
    auto set_window_size = *ASSERT_VAL(bound_sp.make_uniform_setter(glUniform2f, "u_window_size"));

    sl::gfx::buffer<float, sl::gfx::buffer_type::shader_storage, sl::gfx::buffer_usage::dynamic_draw> ssbo;
    ssbo.bind().initialize_data(ssbo_size);

    co_return sl::game::shader{
        .sp{ std::move(sp) },
        .setup{ [ //
                    set_mode = std::move(set_mode),
                    set_time = std::move(set_time),
                    set_window_size = std::move(set_window_size),
                    ssbo = std::move(ssbo),
                    render_entity](
                    sl::ecs::layer& layer, //
                    const sl::game::camera_frame&,
                    const sl::gfx::bound_shader_program& bound_sp
                ) mutable {
            if (auto* state = layer.registry.try_get<RenderState>(render_entity)) {
                state->normalized_freq_proc_output.release().map([&](const std::vector<float>& output) {
                    auto bound_ssbo = ssbo.bind();
                    auto maybe_mapped_ssbo = bound_ssbo.template map<sl::gfx::buffer_access::write_only>();
                    auto mapped_ssbo = *ASSERT_VAL(std::move(maybe_mapped_ssbo));
                    auto mapped_ssbo_data = mapped_ssbo.data();
                    std::copy(output.begin(), output.end(), mapped_ssbo_data.begin());
                });
                state->window_size.release().map([&](const glm::fvec2& window_size) {
                    set_window_size(bound_sp, window_size);
                });
                state->draw_mode.release().map([&](DrawMode draw_mode) {
                    set_mode(bound_sp, static_cast<GLuint>(draw_mode));
                });
                state->time.release().map([&](float time) { set_time(bound_sp, time); });
            }

            return [&, bound_ssbo = ssbo.bind_base(0)] //
                (const sl::gfx::bound_vertex_array& bound_va,
                 sl::game::vertex::draw_type& vertex_draw,
                 std::span<const entt::entity>) {
                    sl::gfx::draw draw{ bound_sp, bound_va };
                    vertex_draw(draw);
                };
        } },
    };
}

sl::exec::async<sl::game::vertex> create_flat_vertex() {
    constexpr std::array flat_vertices{
        V{ +1.0f, +1.0f },
        V{ +1.0f, -1.0f },
        V{ -1.0f, -1.0f },
        V{ -1.0f, +1.0f },
    };
    constexpr std::array flat_indices{
        0u, 1u, 3u, // first triangle
        1u, 2u, 3u, // second triangle
    };

    co_return co_await create_vertex(std::span{ flat_vertices }, std::span{ flat_indices });
}

sl::exec::async<entt::entity>
    create_render_entity(sl::game::engine_context& e_ctx, sl::ecs::layer& layer, glm::ivec2 window_size) {
    using sl::meta::operator""_us;
    auto& us_storage = *layer.registry.get<std::unique_ptr<sl::meta::unique_string_storage>>(layer.root);

    const entt::entity entity = layer.registry.create();

    layer.registry.emplace<sl::game::local_transform>(entity);
    layer.registry.emplace<sl::game::shader::id>(entity, "shader.flat"_us(us_storage));
    layer.registry.emplace<sl::game::vertex::id>(entity, "vertex.flat"_us(us_storage));

    layer.registry.emplace<RenderState>(
        entity,
        RenderState{
            .normalized_freq_proc_output{},
            .window_size{ static_cast<glm::fvec2>(window_size) },
            .draw_mode{ DrawMode::RAY_MARCHING },
            .time{},
        }
    );
    std::ignore = e_ctx.w_ctx.window->frame_buffer_size_cb.connect([&layer, entity](glm::ivec2 frame_buffer_size) {
        layer.registry.get<RenderState>(entity).window_size.set(static_cast<glm::fvec2>(frame_buffer_size));
    });
    layer.registry.emplace<sl::game::overlay>(
        entity,
        [](sl::ecs::layer& layer, sl::gfx::imgui_frame& imgui_frame, entt::entity entity) {
            auto& state = layer.registry.get<RenderState>(entity);
            if (auto imgui_window = imgui_frame.begin("render controls")) {
                ImGui::SetWindowPos(ImVec2{ 350.0f, 0.0f });
                ImGui::SetWindowSize(ImVec2{ 0.0f, 0.0f });

                constexpr auto draw_mode_to_str = [](DrawMode draw_mode) {
                    switch (draw_mode) {
                    case DrawMode::DEFAULT_FILL:
                        return "default fill";
                    case DrawMode::RADIUS_LINEAR:
                        return "radius linear";
                    case DrawMode::RADIUS_LOG:
                        return "radius log";
                    case DrawMode::RAY_MARCHING:
                        return "ray marching";
                    default:
                        break;
                    }
                    return "not implemented";
                };
                const auto preview_draw_mode = state.draw_mode.get().map(draw_mode_to_str).value_or("none");

                if (ImGui::BeginCombo("draw mode", preview_draw_mode)) {
                    for (DrawMode draw_mode{}; draw_mode != DrawMode::ENUM_END; draw_mode = sl::meta::next(draw_mode)) {
                        if (ImGui::Selectable(draw_mode_to_str(draw_mode))) {
                            state.draw_mode.set_if_ne(draw_mode);
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }
    );
    layer.registry.emplace<sl::game::update>(
        entity,
        [](sl::ecs::layer& layer, entt::entity entity, sl::game::time_point time_point) {
            auto& state = layer.registry.get<RenderState>(entity);
            state.time.set(time_point.delta_from_init_sec().count());
        }
    );

    co_return entity;
}

} // namespace visualizer
