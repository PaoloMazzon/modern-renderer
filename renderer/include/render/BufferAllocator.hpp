/// \brief C++ declaration of the temporary buffer abstraction's class
#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace MVRender {
    // MVR_Buffer is a pointer to one of these structs
    struct BufferDescriptor {
        VkBuffer buffer;
        VkDeviceSize offset;
        VkDeviceSize size;
    };

    // For internal use in BufferAllocator
    struct BufferPage {
        VkBuffer vram_buffer; // actual vulkan device memory
        VkBuffer staging_buffer; // staging buffer that will be copied to vram
        VkDeviceSize offset; // current offset for new writes
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

        // How many pages are actually in use
        uint32_t m_page_count = 0;

        // Size of each page
        VkDeviceSize m_page_size = 0;
    public:
        BufferAllocator() = default;
        explicit BufferAllocator(VkDeviceSize page_size);
        ~BufferAllocator();

        // Returns a handle to a temporary buffer of size size and returns a pointer to its first byte of data
        MVR_Buffer allocate_temp_buffer(VkDeviceSize size, void **data);

        // Records necessary copy commands into the copy command buffer
        void record_copy_commands(VkCommandBuffer command_buffer);

        // Performs start of frame tasks.
        void begin_frame();
    };
}