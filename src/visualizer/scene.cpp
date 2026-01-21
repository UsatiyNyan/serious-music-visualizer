//
// Created by usatiynyan.
//

#include "visualizer/scene.hpp"
#include "visualizer/audio.hpp"
#include "visualizer/render.hpp"

#include <sl/meta.hpp>

namespace visualizer {

sl::exec::async<entt::entity> create_global_entity(sl::game::engine_context& e_ctx, sl::ecs::layer& layer) {
    const entt::entity entity = layer.root;

    layer.registry.emplace<GlobalEntityState>(entity);

    layer.registry.emplace<sl::game::local_transform>(entity);
    layer.registry.emplace<sl::game::transform>(entity);
    layer.registry.emplace<sl::game::camera>(entity, sl::game::orthographic_projection{});

    layer.registry.emplace<sl::game::input>(
        entity, [](sl::ecs::layer& layer, const sl::game::input_events& input_events, entt::entity entity) {
            using action = sl::game::keyboard_input_event::action_type;
            auto& state = *ASSERT_VAL((layer.registry.try_get<GlobalEntityState>(entity)));
            const sl::meta::pmatch handle{
                [&state](const sl::game::keyboard_input_event& keyboard) {
                    using key = sl::game::keyboard_input_event::key_type;
                    if (keyboard.key == key::ESCAPE) {
                        const bool prev = state.should_close.get().value_or(false);
                        state.should_close.set(prev || keyboard.action == action::PRESS);
                    }
                },
                [](const auto&) {},
            };
            for (const auto& input_event : input_events) {
                handle(input_event);
            }
        }
    );
    layer.registry.emplace<sl::game::update>(
        entity, [&](sl::ecs::layer& layer, entt::entity entity, sl::game::time_point) {
            auto& state = layer.registry.get<GlobalEntityState>(entity);
            state.should_close.release().map([&cw = e_ctx.w_ctx.current_window](bool should_close) {
                cw.set_should_close(should_close);
            });
        }
    );

    co_return entity;
}

sl::exec::async<void> create_scene(
    sl::game::engine_context& e_ctx,
    sl::ecs::layer& layer,
    const sl::game::basis& world,
    glm::ivec2 window_size
) {
    constexpr audio::DataConfig audio_config{
        /* .capture_channels = */ 1,
        /* .sample_rate = */ 48000,
        /* .frame_count = */ 1024 * 2,
        /* .max_frame_count = */ 1024 * 16,
        /* .frame_window = */ 1024,
    };

    using sl::meta::operator""_us;
    auto& us_storage = layer.registry.emplace<std::unique_ptr<sl::meta::unique_string_storage>>(
        layer.root, std::make_unique<sl::meta::unique_string_storage>()
    );
    ASSERT(us_storage);

    const auto global_entity = co_await create_global_entity(e_ctx, layer);
    const auto render_entity = co_await create_render_entity(e_ctx, layer, window_size);
    sl::game::node::attach_child(layer, global_entity, render_entity);

    {
        const auto audio_entity = co_await create_audio_entity(e_ctx, layer, audio_config, render_entity);
        sl::game::node::attach_child(layer, global_entity, audio_entity);
    }

    {
        auto& shader_resource = layer.registry.emplace<sl::ecs::resource<sl::game::shader>::ptr_type>(
            layer.root, sl::ecs::resource<sl::game::shader>::make(*e_ctx.sync_exec)
        );
        ASSERT(shader_resource);
        auto& vertex_resource = layer.registry.emplace<sl::ecs::resource<sl::game::vertex>::ptr_type>(
            layer.root, sl::ecs::resource<sl::game::vertex>::make(*e_ctx.sync_exec)
        );
        ASSERT(vertex_resource);

        ASSERT(
            co_await shader_resource->require(
                "shader.flat"_us(*us_storage), create_flat_shader(e_ctx, audio_config.frame_count / 2, render_entity)
            )
        );
        ASSERT(co_await vertex_resource->require("vertex.flat"_us(*us_storage), create_flat_vertex()));
    }
}

} // namespace visualizer
