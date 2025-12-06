// Renderer utilities, the core infrastructure is in Renderer.cpp
#include <vulkan/vk_enum_string_helper.h>
#include <fmt/core.h>
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

// TODO: Use something more RAII, or otherwise fix this mess.
MVRender::BufferDescriptor *MVRender::Renderer::load_permanent_buffer(uint64_t size, void *data) {
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

    // Now that we have the memory, we need to map it
    void *mapped_memory;
    VkResult memory_map_result = vmaMapMemory(m_vma, out_stage_allocation, &mapped_memory);

    // If there was a mapping error we will erase it all
    if (memory_map_result != VK_SUCCESS) {
        vmaDestroyBuffer(m_vma, out_stage_buffer, out_stage_allocation);
        vmaDestroyBuffer(m_vma, out_device_buffer, out_device_allocation);
        const char *string_result = string_VkResult(memory_map_result);
        throw Exception(MVR_RESULT_VULKAN_ERROR, fmt::format("Failed to map memory for new page, {}", string_result));
    }

    // Copy data to device buffer
    memcpy(mapped_memory, data, size);
    vmaUnmapMemory(m_vma, out_stage_allocation);
    try {
        VkCommandBuffer command_buffer = get_single_use_command_buffer();
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
        submit_single_use_command_buffer(command_buffer);
        vmaDestroyBuffer(m_vma, out_stage_buffer, out_stage_allocation);
    } catch (MVRender::Exception& r) {
        vmaDestroyBuffer(m_vma, out_stage_buffer, out_stage_allocation);
        vmaDestroyBuffer(m_vma, out_device_buffer, out_device_allocation);
        throw;
    }

    BufferDescriptor *d = get_buffer_descriptor();
    d->size = 0;
    d->offset = 0;
    d->buffer = out_device_buffer;
    d->data = nullptr;
    d->allocation = out_device_allocation;
    return d;
}

void MVRender::Renderer::free_permanent_buffer(BufferDescriptor *buffer) {
    vmaDestroyBuffer(m_vma, buffer->buffer, buffer->allocation);
    remove_buffer_descriptor(buffer);
}