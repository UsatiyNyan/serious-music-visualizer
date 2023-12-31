//
// Created by usatiynyan on 12/22/23.
//

#include "miniaudio/miniaudio.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace ma {

std::string_view result_description(ma_result result) {
    return ma_result_description(result);
}

// ew macros, oh well...
#define MA_UNEXPECTED(x)                                    \
    do {                                                    \
        if (ma_result result = (x); result != MA_SUCCESS) { \
            return tl::make_unexpected(result);             \
        }                                                   \
    } while (false)

tl::expected<context_uptr, ma_result>
    context_init(std::vector<ma_backend> backends, const ma_context_config& context_config) {
    context_uptr context{ new ma_context, &ma_context_uninit };
    const auto* backends_ptr = backends.empty() ? nullptr : backends.data();
    MA_UNEXPECTED(ma_context_init(backends_ptr, backends.size(), &context_config, context.get()));
    return context;
}

tl::expected<device_uptr, ma_result> device_init(const context_uptr& context, const ma_device_config& device_config) {
    device_uptr device{ new ma_device, &ma_device_uninit };
    MA_UNEXPECTED(ma_device_init(context.get(), &device_config, device.get()));
    return device;
}

tl::expected<tl::monostate, ma_result> device_start(const device_uptr& device) {
    MA_UNEXPECTED(ma_device_start(device.get()));
    return tl::monostate{};
}

tl::expected<tl::monostate, ma_result> device_stop(const device_uptr& device) {
    MA_UNEXPECTED(ma_device_stop(device.get()));
    return tl::monostate{};
}

#undef MA_UNEXPECTED

} // namespace ma
