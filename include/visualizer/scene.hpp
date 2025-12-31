//
// Created by usatiynyan.
//

#pragma once

#include <sl/ecs.hpp>
#include <sl/exec.hpp>
#include <sl/game.hpp>
#include <sl/rt.hpp>

namespace visualizer {

struct GlobalEntityState {
    sl::meta::dirty<bool> should_close;
};

sl::exec::async<entt::entity> create_global_entity(sl::game::engine_context& e_ctx, sl::ecs::layer& layer);

sl::exec::async<void> create_scene(
    sl::game::engine_context& e_ctx,
    sl::ecs::layer& layer,
    const sl::game::basis& world,
    glm::ivec2 window_size
);

} // namespace visualizer
