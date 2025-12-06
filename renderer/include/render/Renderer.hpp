/// \brief Singleton renderer that contains all necessary state
#pragma once
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cinttypes>
#include "render/BufferAllocator.hpp"
#include "render/Structs.h"

namespace MVRender {
    // Resources that are per swapchain image
    struct SwapchainResources {
        VkImage image;
        VkImageView image_view;

        // THESE ARE INDEXED VIA (m_frame_counter % m_swapchain_image_count)
        VkSemaphore image_ready_semaphore;
        VkSemaphore submit_ready_semaphore;
    };

    // Resources that are per frame-in-flight
    struct FrameResources {
        VkCommandBuffer copy_commands;
        VkCommandBuffer compute_commands;
        VkCommandBuffer draw_commands;
        BufferAllocator buffer_allocator;
    };

    // Information about the surface
    struct SurfaceFormat {
        uint32_t width;
        uint32_t height;
        uint32_t min_image_count;
        uint32_t max_image_count;
        VkFormat format; // we will simply blit to this at the end of the frame
        VkColorSpaceKHR color_space;
        VkSurfaceCapabilitiesKHR caps;
        bool supports_immediate;
        bool supports_mailbox;
    };

    // Internal renderer state
    class Renderer {
    private:
        Renderer() = default;

        MVR_InitializeParams m_initialize_params{};

        // Internal vulkan state
        VkInstance m_vk_instance;
        VkSurfaceKHR m_vk_surface;
        SurfaceFormat m_surface_format;
        VkPhysicalDevice m_vk_physical_device;
        VkPhysicalDeviceProperties m_vk_physical_device_properties;
        VkDevice m_vk_logical_device;
        VkQueue m_vk_queue; // this is a graphics/compute queue
        VkSwapchainKHR m_vk_swapchain;
        VkCommandPool m_command_pool;
        uint32_t m_queue_family_index;
        uint32_t m_current_sc_image;

        // Synchronization
        uint32_t m_swapchain_image_count;
        uint64_t m_frame_count = 5; // for timeline semaphores
        std::vector<SwapchainResources> m_swapchain_res;
        std::vector<FrameResources> m_frame_res;
        VkSemaphore m_timeline_semaphore;

        // vk-bootstrap state
        vkb::Instance m_vkb_instance;
        vkb::Device m_vkb_logical_device;

        // Memory
        VmaAllocator m_vma;

        // Permanent buffers
        std::vector<BufferDescriptor> m_permanent_buffers;
        std::vector<bool> m_permanent_buffer_occupied;

        // Internal subsystems
        void build_surface_format();

        void initialize_swapchain();
        void quit_swapchain();

        void initialize_sync();
        void quit_sync();

        void initialize_frame_resources();
        void quit_frame_resources();

        void initialize_vma();
        void quit_vma();

        // Returns an empty buffer descriptor stored permanently in the renderer
        BufferDescriptor *get_buffer_descriptor();

        // Invalidates the descriptor, does not free the contents
        void remove_buffer_descriptor(BufferDescriptor *descriptor);

    public:
        // Singleton pattern - the class is destroyed at program end
        static Renderer& instance() {
            static Renderer r;
            return r;
        }

        Renderer(Renderer const&)       = delete;
        void operator=(Renderer const&) = delete;

        // Top-level initialization method
        void initialize_vulkan(MVR_InitializeParams& params);

        // Top-level destruction method
        void quit_vulkan();

        // Util
        [[nodiscard]] VkPresentModeKHR get_present_mode(MVR_PresentMode present_mode) const; // accounts for available present modes
        BufferAllocator &get_buffer_allocator(); // for current frame

        // Internal
        void initialize_instance(); // also creates the device and surface
        void quit_instance();

        void begin_frame();
        void end_frame();

        // Gets a new single-use command buffer
        VkCommandBuffer get_single_use_command_buffer();

        // Submits and waits for a single-use command buffer to finish
        void submit_single_use_command_buffer(VkCommandBuffer buffer);

        // Create and free permanent buffers
        BufferDescriptor *load_permanent_buffer(uint64_t size, void *data);
        void free_permanent_buffer(BufferDescriptor *buffer);
    };
}