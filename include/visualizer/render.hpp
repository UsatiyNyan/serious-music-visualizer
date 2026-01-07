//
// Created by usatiynyan.
//

#pragma once

#include <sl/game.hpp>
#include <sl/gfx.hpp>

namespace visualizer {

struct V {
    sl::gfx::va_attrib_field<2, float> pos;
};

template <typename VT, std::size_t VE, std::unsigned_integral IT, std::size_t IE>
sl::exec::async<sl::game::vertex> create_vertex(std::span<const VT, VE> vertices, std::span<const IT, IE> indices) {
    sl::gfx::vertex_array_builder va_builder;
    va_builder.attributes_from<VT>();
    auto vb = va_builder.buffer<sl::gfx::buffer_type::array, sl::gfx::buffer_usage::static_draw>(vertices);
    auto eb = va_builder.buffer<sl::gfx::buffer_type::element_array, sl::gfx::buffer_usage::static_draw>(indices);
    co_return sl::game::vertex{
        .va = std::move(va_builder).submit(),
        .draw{ [vb = std::move(vb), eb = std::move(eb)](sl::gfx::draw& draw) { draw.elements(eb); } },
    };
}

enum class DrawMode : GLuint {
    RADIUS_LINEAR = 0,
    RADIUS_LOG = 1,
    ENUM_END,
};

struct RenderState {
    sl::meta::dirty<std::vector<float>> normalized_freq_proc_output;
    sl::meta::dirty<glm::fvec2> window_size;
    sl::meta::dirty<DrawMode> draw_mode;
};

sl::exec::async<sl::game::shader>
    create_flat_shader(sl::game::engine_context& e_ctx, std::size_t ssbo_size, entt::entity render_entity);

sl::exec::async<sl::game::vertex> create_flat_vertex();

sl::exec::async<entt::entity>
    create_render_entity(sl::game::engine_context& e_ctx, sl::ecs::layer& layer, glm::ivec2 window_size);

} // namespace visualizer
