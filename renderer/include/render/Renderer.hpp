/// \brief Singleton renderer that contains all necessary state
#pragma once
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <cinttypes>
#include "render/Structs.h"

namespace MVRender {
    // Resources that are per swapchain image
    struct SwapchainResources {
        VkImage swapchain_image;
        VkImageView swapchain_image_view;
        VkSemaphore image_available_semaphore;
    };

    // Resources that are per frame-in-flight
    struct FrameResources {
        VkCommandBuffer copy_commands;
        VkCommandBuffer compute_commands;
        VkCommandBuffer draw_commands;
        VkFence frame_in_use_fence;
        VkSemaphore render_finished_semaphore;
    };

    // Information about the surface
    struct SurfaceFormat {
        uint32_t width;
        uint32_t height;
        uint32_t min_image_count;
        uint32_t max_image_count;
        VkFormat format; // we will simply blit to this at the end of the frame
        VkSurfaceCapabilitiesKHR caps;
        bool supports_immediate;
        bool supports_mailbox;
    };

    // Internal renderer state
    class Renderer {
    private:
        Renderer() {}

        MVR_InitializeParams m_initialize_params;

        // Internal vulkan state
        VkInstance m_vk_instance;
        VkSurfaceKHR m_vk_surface;
        SurfaceFormat m_surface_format;
        VkPhysicalDevice m_vk_physical_device;
        VkDevice m_vk_logical_device;
        VkQueue m_vk_queue; // this is a graphics/compute queue
        VkSwapchainKHR m_vk_swapchain;

        // Synchronization
        std::vector<SwapchainResources> m_swapchain_res;
        std::vector<FrameResources> m_frame_res;

        // vk-bootstrap state
        vkb::Instance m_vkb_instance;
        vkb::Device m_vkb_logical_device;

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
        VkPresentModeKHR get_present_mode(MVR_PresentMode present_mode); // accounts for available present modes

        // Internal
        void initialize_instance(); // also creates the device and surface
        void build_surface_format();
        void initialize_swapchain();
        void quit_swapchain();
        void quit_instance();
    };
}