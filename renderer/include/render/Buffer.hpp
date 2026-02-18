/// \brief Internal representation of permanent and temporary buffers
#pragma once
#include "volk.h"
#include "render/BufferAllocator.hpp"

namespace MVRender {
    // Temporary and permanent buffers
    class Buffer {
    private:
        // If this isn't null, this is a permanent buffer
        VmaAllocation m_allocation;

        VkBuffer m_internal_buffer;
        VkDeviceSize m_size;
        VkDeviceSize m_offset;
        void *m_data;

        // Warns user if a buffer is not freed (and is a permanent buffer)
        bool m_freed = false;
        bool m_not_default = false;

    public:
        Buffer() = default;

        Buffer(VkBuffer internal_buffer, VkDeviceSize size, VkDeviceSize offset, void *data = nullptr, VmaAllocation allocation = nullptr) :
            m_internal_buffer(internal_buffer),
            m_allocation(allocation),
            m_size(size),
            m_offset(offset),
            m_data(data) { m_not_default = true; }

        [[nodiscard]] VkBuffer get_internal_buffer() const { return m_internal_buffer; };
        [[nodiscard]] VkDeviceSize get_size() const { return m_size; };
        [[nodiscard]] VkDeviceSize get_offset() const { return m_offset; };
        [[nodiscard]] VmaAllocation get_allocation() const { return m_allocation; };
        [[nodiscard]] void *get_data() const { return m_data; };
        [[nodiscard]] bool is_permanent() const { return m_allocation != nullptr; }

        // Call this if you free the contents of this buffer, the destructor will complain
        // if this is not manually freed (given the lifecycle tracking of these resources
        // are complicated).
        void mark_freed() { m_freed = true; }

        // This will log an error if the buffer was not already freed, and if it wasn't freed
        // it will wait for device idle and free.
        ~Buffer();
    };
}