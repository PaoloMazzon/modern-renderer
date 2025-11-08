#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include <SDL3/SDL_vulkan.h>
#include <format>
#include <spdlog/spdlog.h>
#include "render/Renderer.hpp"
#include "render/Logging.hpp"

void MVRender::Renderer::initialize_vulkan(MVR_InitializeParams& params) {
    // Get SDL requested layers
    uint32_t count;
    const char * const *extensions = SDL_Vulkan_GetInstanceExtensions(&count);

    // Create instance
    vkb::InstanceBuilder builder;
    builder.set_app_name("MVR")
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0)
            .request_validation_layers(params.debug);
    for (uint32_t i = 0; i < count; i++)
        builder.enable_extension(extensions[i]);
    auto inst_ret = builder.build();
    if (!inst_ret) {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, std::format("Failed to create Vulkan instance, Vulkan error {}", static_cast<uint32_t>(inst_ret.full_error().vk_result)));
    }
    m_vkb_instance = inst_ret.value();
    m_vk_instance = inst_ret.value().instance;

    spdlog::info("Created Vulkan instance.");

    // Create the surface
    if (!SDL_Vulkan_CreateSurface(params.window, m_vk_instance, VK_NULL_HANDLE, &m_vk_surface)) {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_SDL_ERROR, std::format("Failed to create Vulkan surface, SDL error {}", SDL_GetError()));
    }

    spdlog::info("Created Vulkan surface.");

    // Physical device
    vkb::PhysicalDeviceSelector selector { m_vkb_instance };
    auto phys_ret = selector.set_surface (m_vk_surface)
            .set_minimum_version (1, 3)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .select();
    if (!phys_ret) {
        throw MVRender::Exception(MVR_RESULT_NO_DEVICE, std::format("Failed to find a suitable physical device, Vulkan error {}", static_cast<uint32_t>(phys_ret.full_error().vk_result)));
    }
    m_vk_physical_device = phys_ret.value().physical_device;

    spdlog::info("Found suitable physical device {}.", phys_ret.value().name);

    vkb::DeviceBuilder device_builder{ phys_ret.value () };

    // Enable required features
    VkPhysicalDeviceVulkan13Features vulkan13_features = {};
    vulkan13_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13_features.dynamicRendering = VK_TRUE;
    vulkan13_features.synchronization2 = VK_TRUE;

    // Enable bindless
    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features = {};
    descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    descriptor_indexing_features.runtimeDescriptorArray = VK_TRUE;
    descriptor_indexing_features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    descriptor_indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
    descriptor_indexing_features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    descriptor_indexing_features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
    descriptor_indexing_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    descriptor_indexing_features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
    descriptor_indexing_features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;

    // Enable timeline semaphores
    VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore_features = {};
    timeline_semaphore_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    timeline_semaphore_features.timelineSemaphore = VK_TRUE;

    auto dev_ret = device_builder.add_pNext(&vulkan13_features)
            .add_pNext(&descriptor_indexing_features)
            .add_pNext(&timeline_semaphore_features)
            .build();
    if (!dev_ret) {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, std::format("Failed to create the logical device, Vulkan error {}", static_cast<uint32_t>(dev_ret.full_error().vk_result)));
    }
    m_vkb_logical_device = dev_ret.value();
    m_vk_logical_device = m_vkb_logical_device.device;

    spdlog::info("Created logical device.");

    auto graphics_queue_ret = m_vkb_logical_device.get_queue (vkb::QueueType::graphics);
    if (!graphics_queue_ret)  {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, std::format("Failed to create the device queue, Vulkan error {}", static_cast<uint32_t>(graphics_queue_ret.full_error().vk_result)));
    }
    m_vk_queue = graphics_queue_ret.value();

    spdlog::info("Created graphics/compute queue.");
}

void MVRender::Renderer::quit_vulkan() {
    spdlog::info("Waiting for GPU to idle.");
    vkDeviceWaitIdle(m_vk_logical_device);
    vkb::destroy_device(m_vkb_logical_device);
    vkDestroySurfaceKHR(m_vk_instance, m_vk_surface, nullptr);
    vkb::destroy_instance(m_vkb_instance);
    spdlog::info("Freed Vulkan resources.");
}
