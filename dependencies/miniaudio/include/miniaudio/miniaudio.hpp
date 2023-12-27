//
// Created by usatiynyan on 12/22/23.
//

#pragma once

#include "miniaudio.h"

#include <memory>
#include <tl/optional.hpp>
#include <vector>

namespace ma {

using context_uninit = decltype(&ma_context_uninit);
using context_uptr = std::unique_ptr<ma_context, context_uninit>;
context_uptr context_init(std::vector<ma_backend> backends, const ma_context_config& context_config);

using device_uninit = decltype(&ma_device_uninit);
using device_uptr = std::unique_ptr<ma_device, device_uninit>;
device_uptr device_init(const context_uptr& context, const ma_device_config& device_config);

tl::optional<tl::monostate> device_start(const device_uptr& device);
tl::optional<tl::monostate> device_stop(const device_uptr& device);

} // namespace ma
