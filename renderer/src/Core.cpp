#include "render/Core.h"
#include "render/Renderer.hpp"
#include "render/Logging.hpp"

MVR_API MVR_Result mvr_Initialize(MVR_InitializeParams *params) {
    MVR_Result res = MVR_RESULT_SUCCESS;
    try {
        MVRender::Renderer::instance().initialize_vulkan(*params);
    } catch (MVRender::Exception& r) {
        res = r.result();
    }

    return res;
}

MVR_API void mvr_Quit() {
    MVRender::Renderer::instance().quit_vulkan();
}