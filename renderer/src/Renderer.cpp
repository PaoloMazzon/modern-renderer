#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <SDL3/SDL_vulkan.h>
#include <format>
#include <spdlog/spdlog.h>
#include "render/Renderer.hpp"
#include "render/Logging.hpp"

void MVRender::Renderer::m_initialize_vulkan(MVR_InitializeParams& params) {
    // Create instance
    vkb::InstanceBuilder builder;
    auto inst_ret = builder.set_app_name("MVR")
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0)
            .request_validation_layers(params.debug)
            .build();
    if (!inst_ret) {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, std::format("Failed to create Vulkan instance, Vulkan error {}", static_cast<uint32_t>(inst_ret.full_error().vk_result)));
    }
    auto instance_info = inst_ret.value();
    this->vk_instance = inst_ret.value().instance;

    // Create the surface
    if (!SDL_Vulkan_CreateSurface(params.window, this->vk_instance, VK_NULL_HANDLE, &this->vk_surface)) {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_SDL_ERROR, std::format("Failed to create Vulkan surface, SDL error {}", SDL_GetError()));
    }

    // Physical device
    vkb::PhysicalDeviceSelector selector { instance_info };
    auto phys_ret = selector.set_surface (vk_surface)
            .set_minimum_version (1, 3)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .select();
    if (!phys_ret) {
        throw MVRender::Exception(MVR_RESULT_NO_DEVICE, std::format("Failed to find a suitable physical device, Vulkan error {}", static_cast<uint32_t>(phys_ret.full_error().vk_result)));
    }
    this->vk_physical_device = phys_ret.value().physical_device;

    vkb::DeviceBuilder device_builder{ phys_ret.value () };
    auto dev_ret = device_builder.build ();
    if (!dev_ret) {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, std::format("Failed to create the logical device, Vulkan error {}", static_cast<uint32_t>(dev_ret.full_error().vk_result)));
    }
    vkb::Device vkb_device = dev_ret.value ();
    this->vk_logical_device = vkb_device.device;

    auto graphics_queue_ret = vkb_device.get_queue (vkb::QueueType::graphics);
    if (!graphics_queue_ret)  {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, std::format("Failed to create the device queue, Vulkan error {}", static_cast<uint32_t>(graphics_queue_ret.full_error().vk_result)));
    }
    this->vk_queue = graphics_queue_ret.value();
}

void MVRender::Renderer::m_quit_vulkan() {
    // TODO: Free Vulkan resources
}
