#include <string>
#include <vulkan/vk_enum_string_helper.h>
#include <fmt/format.h>
#include "render/Logging.hpp"
#include "render/Core.h"

static thread_local std::string g_error_string = "";

void MVRender::set_error_message(const std::string& msg) {
    g_error_string = "";
    g_error_string.append(msg);
}

void MVRender::resolve_vulkan_error(VkResult result, bool critical, const char *msg) {
    if (result < 0) {
        const char *string_result = string_VkResult(result);
        throw MVRender::Exception(critical ? MVR_RESULT_CRITICAL_VULKAN_ERROR : MVR_RESULT_VULKAN_ERROR, fmt::format("{}, Vulkan error {}", msg, string_result));
    }
}

MVR_API const char *mvr_GetError() {
    return g_error_string.c_str();
}
