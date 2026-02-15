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

    public:
        Buffer() = delete;

        // Create a temporary buffer
        Buffer(VkBuffer internal_buffer, VkDeviceSize size, VkDeviceSize offset, VmaAllocation allocation = nullptr);

        VkBuffer get_internal_buffer();
        VkDeviceSize get_size();
        VkDeviceSize get_offset();
        VmaAllocation get_allocation();

        [[nodiscard]] bool is_permanent() const {
            return m_allocation != nullptr;
        }
    };
}