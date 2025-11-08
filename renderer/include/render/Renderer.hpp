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
        VkInstance m_vk_instance;
        VkSurfaceKHR m_vk_surface;
        VkPhysicalDevice m_vk_physical_device;
        VkDevice m_vk_logical_device;
        VkQueue m_vk_queue; // this is a graphics/compute queue

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

        // Internal rendering methods
        void initialize_vulkan(MVR_InitializeParams& params);
        void quit_vulkan();
    };
}