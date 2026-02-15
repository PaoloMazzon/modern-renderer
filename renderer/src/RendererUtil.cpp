// Renderer utilities, the core infrastructure is in Renderer.cpp
#define VK_NO_PROTOTYPES
#include <volk.h>
#include <vulkan/vk_enum_string_helper.h>
#include <fmt/core.h>

#include "render/Renderer.hpp"
#include "render/BufferAllocator.hpp"
#include "render/Constants.hpp"
#include "render/Logging.hpp"

MVRender::BufferAllocator &MVRender::Renderer::get_buffer_allocator() {
    return m_frame_res.at(m_frame_count % FRAMES_IN_FLIGHT).buffer_allocator;
}

VkPresentModeKHR MVRender::Renderer::get_present_mode(MVR_PresentMode present_mode) const {
    if (present_mode == MVR_PRESENT_MODE_IMMEDIATE && m_surface_format.supports_immediate)
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    if (present_mode == MVR_PRESENT_MODE_TRIPLE_BUFFER && m_surface_format.supports_mailbox)
        return VK_PRESENT_MODE_MAILBOX_KHR;
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkCommandBuffer MVRender::Renderer::get_single_use_command_buffer() {
    VkCommandBufferAllocateInfo allocate_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
    };
    VkCommandBuffer buffer;
    VkResult allocate_result = vkAllocateCommandBuffers(m_vk_logical_device, &allocate_info, &buffer);
    resolve_vulkan_error(allocate_result, false, "Failed to allocate single-use command buffer");

    VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    VkResult begin_result = vkBeginCommandBuffer(buffer, &begin_info);
    resolve_vulkan_error(begin_result, false, "Failed to begin single-use command buffer");

    return buffer;
}

void MVRender::Renderer::submit_single_use_command_buffer(VkCommandBuffer buffer) {
    VkResult end_result = vkEndCommandBuffer(buffer);
    resolve_vulkan_error(end_result, false, "Failed to begin single-use command buffer");

    VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &buffer
    };
    vkQueueSubmit(m_vk_queue, 1, &submit_info, nullptr);

    // TODO: Something better than this pile of shit
    vkQueueWaitIdle(m_vk_queue);

    vkFreeCommandBuffers(m_vk_logical_device, m_command_pool, 1, &buffer);
}

// TODO: Use something more RAII, or otherwise fix this mess.
MVRender::BufferDescriptor *MVRender::Renderer::load_permanent_buffer(uint64_t size, void *data) {
    // TODO: Use BufferAllocator as a staging buffer and use it's copy command buffer to perform the copy
    return nullptr;
}

void MVRender::Renderer::free_permanent_buffer(BufferDescriptor *buffer) {
    // TODO: Add this to this FIF's free list
}

void MVRender::Renderer::debug_name_object(uint64_t object, VkObjectType type, const std::string& name) {
    if (!m_debug_names_enabled) return;
    VkDebugUtilsObjectNameInfoEXT name_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType = type,
            .objectHandle = object,
            .pObjectName = name.c_str(),
    };
    m_fp.fn_vkSetDebugUtilsObjectNameEXT(m_vk_logical_device, &name_info);
}
