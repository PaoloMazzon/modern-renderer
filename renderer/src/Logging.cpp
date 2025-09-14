#include <string>
#include <mutex>
#include "render/Logging.hpp"
#include "render/Core.h"

static thread_local std::string g_error_string = "";

void Internal_Render::set_error_message(const char *msg) {
    g_error_string = "";
    g_error_string.append(msg);
}

MVR_API const char *mvr_GetError() {
    return g_error_string.c_str();
}
