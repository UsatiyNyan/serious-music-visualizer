//
// Created by usatiynyan.
//

#include "visualizer/scene.hpp"

#include <sl/ecs.hpp>
#include <sl/exec.hpp>
#include <sl/game.hpp>
#include <sl/rt.hpp>

#include <sl/meta/assert.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>

int main(int argc, char** argv) {
    auto a_logger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sl::game::logger().set_level(spdlog::level::debug);
    sl::game::logger().sinks().push_back(a_logger);
    sl::gfx::logger().set_level(spdlog::level::debug);
    sl::gfx::logger().sinks().push_back(a_logger);

    const glm::ivec2 window_size{ 1280, 720 };
    auto w_ctx = *ASSERT_VAL(sl::game::window_context::initialize(
        sl::gfx::context::options{ 3, 3, GLFW_OPENGL_CORE_PROFILE },
        "serious-music-visualizer",
        window_size,
        { 0.1f, 0.1f, 0.1f, 0.1f }
    ));
    auto e_ctx = sl::game::engine_context::initialize(std::move(w_ctx), argc, argv);
    sl::ecs::layer layer{};
    sl::game::graphics_system gfx_system{ .layer = layer, .world{} };
    sl::game::overlay_system overlay_system{ .layer = layer };
    sl::gfx::implot_context implot{ e_ctx.w_ctx.imgui };

    sl::exec::coro_schedule(*e_ctx.script_exec, visualizer::create_scene(e_ctx, layer, gfx_system.world, window_size));

    while (e_ctx.is_ok()) {
        while (e_ctx.script_exec->execute_batch() > 0) {}
        e_ctx.spin_once(layer, gfx_system, overlay_system);
    }

    while (e_ctx.script_exec->execute_batch() > 0) {}
}
