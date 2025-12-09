/// \brief
#pragma once
#include <volk.h>

// These are function pointers for extensions we are using
struct VulkanFunctionPointers {
    PFN_vkSetDebugUtilsObjectNameEXT fn_vkSetDebugUtilsObjectNameEXT;
};