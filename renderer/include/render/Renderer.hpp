/// \brief Singleton renderer that contains all necessary state
#pragma once
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include "render/Structs.h"

namespace MVRender {
    // Internal renderer state
    class Renderer {
    private:
        Renderer() {}

        // Internal vulkan state
        VkInstance vk_instance;
        VkSurfaceKHR vk_surface;
        VkPhysicalDevice vk_physical_device;
        VkDevice vk_logical_device;
        VkQueue vk_queue; // this is a graphics/compute queue

        // vk-bootstrap state
        vkb::Instance vkb_instance;
        vkb::Device vkb_logical_device;

    public:
        // Singleton pattern - the class is destroyed at program end
        static Renderer& instance() {
            static Renderer r;
            return r;
        }

        Renderer(Renderer const&)       = delete;
        void operator=(Renderer const&) = delete;

        // Internal rendering methods
        void m_initialize_vulkan(MVR_InitializeParams& params);
        void m_quit_vulkan();
    };
}