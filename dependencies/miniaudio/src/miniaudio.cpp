//
// Created by usatiynyan on 12/22/23.
//

#include "miniaudio/miniaudio.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace ma {

context_uptr context_init(std::vector<ma_backend> backends, const ma_context_config& context_config) {
    context_uptr context{ new ma_context, &ma_context_uninit };
    const auto* backends_ptr = backends.empty() ? nullptr : backends.data();
    if (ma_context_init(backends_ptr, backends.size(), &context_config, context.get()) != MA_SUCCESS) {
        context.reset();
    }
    return context;
}

device_uptr device_init(const context_uptr& context, const ma_device_config& device_config) {
    device_uptr device{ new ma_device, &ma_device_uninit };
    if (ma_device_init(context.get(), &device_config, device.get()) != MA_SUCCESS) {
        device.reset();
    }
    return device;
}

tl::optional<tl::monostate> device_start(const device_uptr& device) {
    return ma_device_start(device.get()) == MA_SUCCESS ? tl::optional{ tl::monostate{} } : tl::nullopt;
}

tl::optional<tl::monostate> device_stop(const device_uptr& device) {
    return ma_device_stop(device.get()) == MA_SUCCESS ? tl::optional{ tl::monostate{} } : tl::nullopt;
}

} // namespace ma
