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
MVRender::Buffer *MVRender::Renderer::load_permanent_buffer(uint64_t size, void *data) {
    // TODO: Create a transfer and permanent buffer. That transfer buffer should then be
    //       added to this FIF's free list, and the copy command should be recorded on this
    //       FIF's buffer allocator's copy command buffer.
    static uint32_t index = 0;
    index += 1;

    // Create the staging buffer
    VkBuffer out_stage_buffer;
    VmaAllocation out_stage_allocation;
    VkBufferCreateInfo staging_buffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &m_queue_family_index,
    };
    VmaAllocationCreateInfo staging_allocation_create_info = {
            .flags = 0,
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    };
    VmaAllocationInfo stage_allocation_info;
    VkResult stage_buffer_result = vmaCreateBuffer(m_vma, &staging_buffer_create_info,
                                                   &staging_allocation_create_info, &out_stage_buffer, &out_stage_allocation, &stage_allocation_info);

    if (stage_buffer_result != VK_SUCCESS) {
        const char *string_result = string_VkResult(stage_buffer_result);
        throw Exception(MVR_RESULT_VULKAN_ERROR, fmt::format("Failed to allocate staging buffer for new page, {}", string_result));
    }

    // Create the device buffer
    VkBuffer out_device_buffer;
    VmaAllocation out_device_allocation;
    VkBufferCreateInfo device_buffer_create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = size,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
            .queueFamilyIndexCount = 1,
            .pQueueFamilyIndices = &m_queue_family_index,
    };
    VmaAllocationCreateInfo device_allocation_create_info = {
            .usage = VMA_MEMORY_USAGE_GPU_TO_CPU,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    VmaAllocationInfo device_allocation_info;
    VkResult device_buffer_result = vmaCreateBuffer(m_vma, &device_buffer_create_info,
                                                    &device_allocation_create_info, &out_device_buffer, &out_device_allocation, &device_allocation_info);

    if (device_buffer_result != VK_SUCCESS) {
        // Gotta throw out the staging buffer
        vmaDestroyBuffer(m_vma, out_stage_buffer, out_stage_allocation);

        const char *string_result = string_VkResult(device_buffer_result);
        throw Exception(MVR_RESULT_VULKAN_ERROR, fmt::format("Failed to allocate device buffer for new page, {}", string_result));
    }

    debug_name_object(
            reinterpret_cast<uint64_t>(out_device_buffer),
            VK_OBJECT_TYPE_BUFFER,
            fmt::format("Permanent buffer {}", index)
    );

    // Now that we have the memory, we need to map it
    void *mapped_memory;
    VkResult memory_map_result = vmaMapMemory(m_vma, out_stage_allocation, &mapped_memory);

    // If there was a mapping error we will erase it all
    if (memory_map_result != VK_SUCCESS) {
        vmaDestroyBuffer(m_vma, out_stage_buffer, out_stage_allocation);
        vmaDestroyBuffer(m_vma, out_device_buffer, out_device_allocation);
        const char *string_result = string_VkResult(memory_map_result);
        throw Exception(MVR_RESULT_VULKAN_ERROR, fmt::format("Failed to map memory for transfer buffer, {}", string_result));
    }

    // Copy data to device buffer
    FrameResources *frame = &m_frame_res[m_frame_count % FRAMES_IN_FLIGHT];
    memcpy(mapped_memory, data, size);
    vmaUnmapMemory(m_vma, out_stage_allocation);
    VkCommandBuffer command_buffer = frame->copy_commands;
    VkBufferCopy2 region = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
            .size = size,
    };
    VkCopyBufferInfo2 copy_buffer_info = {
            .sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
            .srcBuffer = out_stage_buffer,
            .dstBuffer = out_device_buffer,
            .regionCount = 1,
            .pRegions = &region,
    };
    vkCmdCopyBuffer2(command_buffer, &copy_buffer_info);

    uint32_t stage_index, device_index;
    std::optional<Buffer> *staging_buffer_slot = get_permanent_buffer_slot(&stage_index);
    staging_buffer_slot->emplace(
            out_stage_buffer,
            size,
            0,
            nullptr,
            out_stage_allocation);
    std::optional<Buffer> *buffer_slot = get_permanent_buffer_slot(&device_index);
    buffer_slot->emplace(
            out_device_buffer,
            size,
            0,
            nullptr,
            out_device_allocation);

    frame->free_list.push_back(stage_index);
    return &buffer_slot->value();
}

std::optional<MVRender::Buffer> *MVRender::Renderer::get_permanent_buffer_slot(uint32_t *out_index) {
    *out_index = 0;
    for (auto & m_permanent_buffer : m_permanent_buffers) {
        if (!m_permanent_buffer.has_value()) {
            return &m_permanent_buffer;
        }
        (*out_index)++;
    }
    m_permanent_buffers.emplace_back(std::nullopt);
    return &m_permanent_buffers[*out_index];
}

void MVRender::Renderer::free_permanent_buffer(Buffer *buffer) {
    for (uint32_t i = 0; i < m_permanent_buffers.size(); i++) {
        if (m_permanent_buffers[i].has_value() && &m_permanent_buffers[i].value() == buffer) {
            m_frame_res[m_frame_count % FRAMES_IN_FLIGHT].free_list.push_back(i);
            return;
        }
    }
    // This means you tried to free a buffer that doesn't exist
    MVR_EXPRESSION_UNREACHABLE
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
