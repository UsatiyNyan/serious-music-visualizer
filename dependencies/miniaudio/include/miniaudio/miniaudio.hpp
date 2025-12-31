//
// Created by usatiynyan
//

#pragma once

#include "miniaudio.h"

#include <sl/meta/monad/result.hpp>
#include <sl/meta/type/unit.hpp>

#include <memory>
#include <span>
#include <vector>

namespace ma {

std::string_view result_description(ma_result result);
std::string_view get_backend_name(ma_backend backend);

sl::meta::result<std::vector<ma_backend>, ma_result> get_enabled_backends();

using context_uninit = decltype(&ma_context_uninit);
using context_uptr = std::unique_ptr<ma_context, context_uninit>;
sl::meta::result<context_uptr, ma_result>
    context_init(const std::vector<ma_backend>& backends, const ma_context_config& context_config);

struct context_get_devices_result_t {
    std::span<ma_device_info> playback_infos;
    std::span<ma_device_info> capture_infos;
};

sl::meta::result<context_get_devices_result_t, ma_result> context_get_devices(const context_uptr& context);

using device_uninit = decltype(&ma_device_uninit);
using device_uptr = std::unique_ptr<ma_device, device_uninit>;
sl::meta::result<device_uptr, ma_result>
    device_init(const context_uptr& context, const ma_device_config& device_config);

sl::meta::result<sl::meta::unit, ma_result> device_start(const device_uptr& device);
sl::meta::result<sl::meta::unit, ma_result> device_stop(const device_uptr& device);

} // namespace ma
