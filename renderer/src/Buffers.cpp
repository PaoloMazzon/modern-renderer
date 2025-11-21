#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <fmt/core.h>
#include "render/Renderer.hpp"
#include "render/BufferAllocator.hpp"
#include "render/Buffers.h"
#include "render/Logging.hpp"

void MVRender::BufferAllocator::append_page(VkDeviceSize size) {
    auto &renderer = MVRender::Renderer::instance();
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
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
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
        const char *string_result = string_VkResult(device_buffer_result);
        throw Exception(MVR_RESULT_VULKAN_ERROR, fmt::format("Failed to allocate device buffer for new page, {}", string_result));
    }

    BufferPage page = {
        .vram_buffer = out_device_buffer,
        .vram_allocation = out_device_allocation,
        .staging_buffer = out_stage_buffer,
        .staging_allocation = out_stage_allocation,
        .offset = 0,
        .size = size,
        .data = stage_allocation_info.pMappedData,
    };

    m_buffer_pages.emplace_back(page);
}

MVRender::BufferPage *MVRender::BufferAllocator::find_page(VkDeviceSize size) {
    // Try and find a page with available space
    BufferPage *found_page = nullptr;
    for (auto &page: m_buffer_pages) {
        if (page.size - page.offset >= size) {
            found_page = &page;
            break;
        }
    }

    // Create a new page if there isn't already an available page
    if (found_page == nullptr) {
        VkDeviceSize page_size = size > m_page_size ? m_page_size + size : m_page_size;
        append_page(page_size);
        found_page = &m_buffer_pages.at(m_buffer_pages.size() - 1);
    }
    return found_page;
}

// This will return current + size, where size is rounded up to the nearest alignment
static VkDeviceSize move_by_alignment(VkDeviceSize current, VkDeviceSize increase, VkDeviceSize alignment) {
    if (increase % alignment != 0) {
        VkDeviceSize additional = alignment - (increase % alignment);
        return current + increase + additional;
    }
    return current + increase;
}

MVRender::BufferDescriptor *MVRender::BufferAllocator::get_buffer_descriptor(VkDeviceSize size) {
    BufferPage *page = find_page(size);

    BufferDescriptor descriptor = {
        .buffer = page->vram_buffer,
        .offset = page->offset,
        .size = size,
        .data = static_cast<uint8_t*>(page->data) + page->offset,
    };
    m_buffers.emplace_back(descriptor);

    page->offset = move_by_alignment(page->offset, size, m_minimum_alignment);

    return &m_buffers.back();
}

MVRender::BufferAllocator::BufferAllocator(MVRender::BufferAllocatorCreateInfo &create_info) {
    m_page_size = create_info.page_size;
    m_vma = create_info.allocator;
    m_logical_device = create_info.logical_device;
    m_queue_family_index = create_info.queue_family_index;

    // We need to find the minimum alignment we care about (minimum being the highest value
    // of all the alignments of the types of data this thing supports)
    VkDeviceSize align1 = create_info.device_properties.limits.minStorageBufferOffsetAlignment;
    VkDeviceSize align2 = create_info.device_properties.limits.minTexelBufferOffsetAlignment;
    m_minimum_alignment = align1 > align2 ? align1 : align2;
}

MVRender::BufferAllocator::~BufferAllocator() {
    for (auto page: m_buffer_pages) {
        vmaDestroyBuffer(m_vma, page.vram_buffer, page.vram_allocation);
        vmaDestroyBuffer(m_vma, page.staging_buffer, page.staging_allocation);
    }
}

MVR_Buffer MVRender::BufferAllocator::allocate_temp_buffer(VkDeviceSize size, void **data) {
    BufferDescriptor *descriptor = get_buffer_descriptor(size);
    *data = descriptor->data;
    return reinterpret_cast<MVR_Buffer>(descriptor);
}

void MVRender::BufferAllocator::record_copy_commands(VkCommandBuffer command_buffer) {
    // TODO: This
}

void MVRender::BufferAllocator::begin_frame() {
    // TODO: This
}

MVR_API MVR_Result mvr_CreateTempBuffer(uint64_t size, void *data, MVR_Buffer *buffer) {
    // TODO: This
    return MVR_RESULT_FAILURE;
}

MVR_API MVR_Result mvr_AllocateTempBuffer(uint64_t size, void **data, MVR_Buffer *buffer) {
    MVR_Result status = MVR_RESULT_SUCCESS;
    try {
        auto &instance = MVRender::Renderer::instance();
        *buffer = instance.get_buffer_allocator().allocate_temp_buffer(size, data);
    } catch (MVRender::Exception& r) {
        status = r.result();
        *data = nullptr;
        *buffer = MVR_INVALID_HANDLE;
    }
    return status;
}

MVR_API MVR_Result mvr_CreateBuffer(uint64_t size, void *data, MVR_Buffer *buffer) {
    // TODO: This
    return MVR_RESULT_FAILURE;
}

MVR_API MVR_Result mvr_AllocateBuffer(uint64_t size, void **data, MVR_Buffer *buffer) {
    // TODO: This
    return MVR_RESULT_FAILURE;
}

MVR_API void mvr_DestroyBuffer(MVR_Buffer buffer) {
    // TODO: This
}