#include "render/Core.h"
#include "render/Renderer.hpp"

MVR_API MVR_Result mvr_Initialize(MVR_InitializeParams *params) {
    MVR_Result res = MVR_RESULT_SUCCESS;
    try {
        Internal_Render::Renderer::instance().m_initialize_vulkan(*params);
    } catch (MVR_Result r) {
        res = r;
    }

    return res;
}

MVR_API void mvr_Quit() {

}