// Renderer utilities, the core infrastructure is in Renderer.cpp
#include "render/Renderer.hpp"
#include "render/BufferAllocator.hpp"
#include "render/Constants.hpp"
#include "render/Logging.hpp"

MVRender::BufferDescriptor *MVRender::Renderer::get_buffer_descriptor() {
    int found_spot = -1;
    for (int i = 0; i < m_permanent_buffer_occupied.size(); i++) {
        if (!m_permanent_buffer_occupied.at(i)) {
            found_spot = i;
            m_permanent_buffer_occupied.at(i) = true;
            break;
        }
    }

    if (found_spot == -1) {
        m_permanent_buffer_occupied.emplace_back(true);
        m_permanent_buffers.emplace_back();
        found_spot = m_permanent_buffer_occupied.size() - 1;
    }
    return &m_permanent_buffers.at(found_spot);
}

void MVRender::Renderer::remove_buffer_descriptor(MVRender::BufferDescriptor *descriptor) {
    for (int i = 0; i < m_permanent_buffer_occupied.size(); i++) {
        if (&m_permanent_buffers.at(i) == descriptor) {
            m_permanent_buffer_occupied.at(i) = false;
            break;
        }
    }
}

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

MVRender::BufferDescriptor *MVRender::Renderer::load_permanent_buffer(uint64_t size, void *data) {
    // TODO: This
}

void MVRender::Renderer::free_permanent_buffer(BufferDescriptor *buffer) {
    // TODO: This
}