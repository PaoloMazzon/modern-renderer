#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <SDL3/SDL_vulkan.h>
#include <format>
#include "render/Renderer.hpp"
#include "render/Logging.hpp"

void Internal_Render::Renderer::m_initialize_vulkan(MVR_InitializeParams& params) {
    // Create instance
    vkb::InstanceBuilder builder;
    builder.set_app_name("MVR")
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0);
    if (params.debug)
        builder.request_validation_layers();
    auto inst_ret = builder.build();
    if (!inst_ret) {
        Internal_Render::set_error_message(std::format("Failed to create Vulkan instance, Vulkan error {}", static_cast<uint32_t>(inst_ret.full_error().vk_result)));
        throw MVR_RESULT_CRITICAL_VULKAN_ERROR;
    }
    auto instance_info = inst_ret.value();
    this->vk_instance = inst_ret.value().instance;

    // Create the surface
    if (!SDL_Vulkan_CreateSurface(params.window, this->vk_instance, VK_NULL_HANDLE, &this->vk_surface)) {
        Internal_Render::set_error_message(std::format("Failed to create Vulkan surface, SDL error {}", SDL_GetError()));
        throw MVR_RESULT_CRITICAL_SDL_ERROR;
    }

    // Physical device
    vkb::PhysicalDeviceSelector selector { instance_info };
    auto phys_ret = selector.set_surface (vk_surface)
            .set_minimum_version (1, 3)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .select();
    if (!phys_ret) {
        Internal_Render::set_error_message(std::format("Failed to find a suitable physical device, Vulkan error {}", static_cast<uint32_t>(phys_ret.full_error().vk_result)));
        throw MVR_RESULT_NO_DEVICE;
    }
    this->vk_physical_device = phys_ret.value().physical_device;

    vkb::DeviceBuilder device_builder{ phys_ret.value () };
    auto dev_ret = device_builder.build ();
    if (!dev_ret) {
        Internal_Render::set_error_message(std::format("Failed to create the logical device, Vulkan error {}", static_cast<uint32_t>(dev_ret.full_error().vk_result)));
        throw MVR_RESULT_CRITICAL_VULKAN_ERROR;
    }
    vkb::Device vkb_device = dev_ret.value ();
    this->vk_logical_device = vkb_device.device;

    auto graphics_queue_ret = vkb_device.get_queue (vkb::QueueType::graphics);
    if (!graphics_queue_ret)  {
        Internal_Render::set_error_message(std::format("Failed to create the device queue, Vulkan error {}", static_cast<uint32_t>(graphics_queue_ret.full_error().vk_result)));
        throw MVR_RESULT_CRITICAL_VULKAN_ERROR;
    }
    this->vk_queue = graphics_queue_ret.value();
}

void Internal_Render::Renderer::m_quit_vulkan() {
    // TODO: Free Vulkan resources
}
