/// \brief C++ declaration of the temporary buffer abstraction's class
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "render/Structs.h"

namespace MVRender {
    struct BufferAllocatorCreateInfo {
        VmaAllocator allocator;
        VkDevice logical_device;
        VkDeviceSize page_size;
        uint32_t queue_family_index;
        VkPhysicalDeviceProperties device_properties;
        uint32_t frame_in_flight_index;
    };

    // MVR_Buffer is a pointer to one of these structs. Permanent buffers do not care about anything except buffer.
    struct BufferDescriptor {
        VkBuffer buffer; // the device-local buffer
        VmaAllocation allocation; // allocation for permanent buffers -- NOT FOR TEMPORARY
        VkDeviceSize offset; // offset in that buffer for this virtual buffer
        VkDeviceSize size; // amount of bytes pertaining to this buffer
        void *data; // memory-mapped host-visible pointer to the start of the range
    };

    // For internal use in BufferAllocator
    struct BufferPage {
        VkBuffer vram_buffer; // actual vulkan device memory
        VmaAllocation vram_allocation;
        VkBuffer staging_buffer; // staging buffer that will be copied to vram
        VmaAllocation staging_allocation;
        VkDeviceSize offset; // current offset for new writes
        VkDeviceSize size; // size of this page
        void *data; // data for the staging buffer
    };

    // A paging temporary buffer allocator. There should be one of these
    // per frame in flight to allow users to allocate arbitrary temporary
    // buffers in vram.
    class BufferAllocator {
        // List of temporary buffers
        std::vector<BufferDescriptor> m_buffers;

        // Pages of memory, both the staging and device memory
        std::vector<BufferPage> m_buffer_pages;

        // Size of each page by default
        VkDeviceSize m_page_size = 0;

        // Internal Vulkan handles
        VmaAllocator m_vma;
        VkDevice m_logical_device;
        uint32_t m_queue_family_index;
        uint32_t m_index; // frame in flight index for debug purposes

        // Minimum required alignment for memory to be placed in pages
        VkDeviceSize m_minimum_alignment = 0;

        // Allocates and appends a new page to the allocator, can fail
        void append_page(VkDeviceSize size);

        // Finds/creates a page with at least size size, can fail
        BufferPage *find_page(VkDeviceSize size);

        // Attempts to get a buffer out of the allocator, can fail
        BufferDescriptor *get_buffer_descriptor(VkDeviceSize size);
    public:
        BufferAllocator() = default;
        explicit BufferAllocator(BufferAllocatorCreateInfo &create_info);
        ~BufferAllocator();

        // Returns a handle to a temporary buffer of size size and returns a pointer to its first byte of data
        MVR_Buffer allocate_temp_buffer(VkDeviceSize size, void **data);

        // Returns a handle to a permanent buffer of size size, requires a pointer to a buffer descriptor to fill
        MVR_Buffer allocate_permanent_buffer(VkDeviceSize size, void *data);

        // Records necessary copy commands into the copy command buffer
        void record_copy_commands(VkCommandBuffer command_buffer);

        // Performs start of frame tasks.
        void begin_frame();
    };
}