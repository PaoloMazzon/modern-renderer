#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <VkBootstrap.h>
#include <SDL3/SDL_vulkan.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "render/Renderer.hpp"
#include "render/Logging.hpp"
#include "render/Constants.hpp"

void MVRender::Renderer::initialize_vulkan(MVR_InitializeParams& params) {
    m_initialize_params = params;
    initialize_instance();
    build_surface_format();
    initialize_swapchain();
    initialize_sync();
    initialize_vma();
    initialize_frame_resources();
    begin_frame();
    spdlog::info("Finished initializing renderer.");
}

void MVRender::Renderer::quit_vulkan() {
    spdlog::info("Waiting for GPU to idle.");
    end_frame();
    vkDeviceWaitIdle(m_vk_logical_device);

    // Destroy subsystems
    quit_frame_resources();
    quit_vma();
    quit_sync();
    quit_swapchain();
    quit_instance();

    spdlog::info("Freed Vulkan resources.");
}

MVRender::BufferAllocator &MVRender::Renderer::get_buffer_allocator() {
    return m_frame_res.at(m_frame_count % FRAMES_IN_FLIGHT).buffer_allocator;
}

void MVRender::Renderer::initialize_instance() {
    // Get SDL requested layers
    uint32_t count;
    const char * const *extensions = SDL_Vulkan_GetInstanceExtensions(&count);

    // Create instance
    vkb::InstanceBuilder builder;
    builder.set_app_name("MVR")
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0)
            .request_validation_layers(m_initialize_params.debug);
    for (uint32_t i = 0; i < count; i++)
        builder.enable_extension(extensions[i]);
    auto inst_ret = builder.build();
    if (!inst_ret) {
        const char *string_result = string_VkResult(inst_ret.vk_result());
        throw MVRender::Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create Vulkan instance, Vulkan error {}", string_result));
    }
    m_vkb_instance = inst_ret.value();
    m_vk_instance = inst_ret.value().instance;

    spdlog::info("Created Vulkan instance.");

    // Create the surface
    if (!SDL_Vulkan_CreateSurface(m_initialize_params.window, m_vk_instance, VK_NULL_HANDLE, &m_vk_surface)) {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_SDL_ERROR, fmt::format("Failed to create Vulkan surface, SDL error {}", SDL_GetError()));
    }

    spdlog::info("Created Vulkan surface.");

    // Physical device
    vkb::PhysicalDeviceSelector selector { m_vkb_instance };
    auto phys_ret = selector.set_surface (m_vk_surface)
            .set_minimum_version (1, 3)
            .prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .select();
    if (!phys_ret) {
        throw MVRender::Exception(MVR_RESULT_NO_DEVICE, fmt::format("Failed to find a suitable physical device, Vulkan error {}", static_cast<uint32_t>(phys_ret.full_error().vk_result)));
    }
    m_vk_physical_device = phys_ret.value().physical_device;
    m_vk_physical_device_properties = phys_ret->properties;

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
        const char *string_result = string_VkResult(dev_ret.vk_result());
        throw MVRender::Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create the logical device, Vulkan error {}", string_result));
    }
    m_vkb_logical_device = dev_ret.value();
    m_vk_logical_device = m_vkb_logical_device.device;

    spdlog::info("Created logical device.");

    auto graphics_queue_ret = m_vkb_logical_device.get_queue (vkb::QueueType::graphics);
    if (!graphics_queue_ret)  {
        const char *string_result = string_VkResult(graphics_queue_ret.vk_result());
        throw MVRender::Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create the device queue, Vulkan error {}", string_result));
    }
    m_vk_queue = graphics_queue_ret.value();
    m_queue_family_index = m_vkb_logical_device.get_queue_index(vkb::QueueType::graphics).value();

    spdlog::info("Created graphics/compute queue.");
}

void MVRender::Renderer::quit_instance() {
    vkb::destroy_device(m_vkb_logical_device);
    vkDestroySurfaceKHR(m_vk_instance, m_vk_surface, nullptr);
    vkb::destroy_instance(m_vkb_instance);

    spdlog::info("Freed logical device, surface, and instance.");
}

void MVRender::Renderer::build_surface_format() {
    // Get the surface formats
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_vk_physical_device, m_vk_surface, &format_count, nullptr);
    std::vector<VkSurfaceFormatKHR> surface_formats;
    surface_formats.resize(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_vk_physical_device, m_vk_surface, &format_count, surface_formats.data());

    // According to the hardware database 99.5% of devices have an sRGB format so we'll require it
    VkSurfaceFormatKHR surface_format = {};
    bool found_srgb = false;
    for (auto format: surface_formats) {
        if ((format.format == VK_FORMAT_B8G8R8A8_SRGB || format.format == VK_FORMAT_R8G8B8A8_SRGB) &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = format;
            found_srgb = true;
            break;
        }
    }
    if (!found_srgb) {
        throw Exception(MVR_RESULT_NO_DEVICE, "The device surface does not support an sRGB format.");
    }

    // And fill out capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vk_physical_device, m_vk_surface, &m_surface_format.caps);
    m_surface_format.width = m_surface_format.caps.currentExtent.width;
    m_surface_format.height = m_surface_format.caps.currentExtent.height;
    m_surface_format.max_image_count = m_surface_format.caps.maxImageCount;
    m_surface_format.min_image_count = m_surface_format.caps.minImageCount;
    m_surface_format.format = surface_format.format;
    m_surface_format.color_space = surface_format.colorSpace;

    // Find present modes
    std::vector<VkPresentModeKHR> present_modes;
    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_vk_physical_device, m_vk_surface, &present_mode_count, nullptr);
    present_modes.resize(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_vk_physical_device, m_vk_surface, &present_mode_count, present_modes.data());
    for (auto present_mode: present_modes) {
        if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            m_surface_format.supports_immediate = true;
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            m_surface_format.supports_mailbox = true;
    }

    spdlog::info("Built surface format information.");
}

VkPresentModeKHR MVRender::Renderer::get_present_mode(MVR_PresentMode present_mode) const {
    if (present_mode == MVR_PRESENT_MODE_IMMEDIATE && m_surface_format.supports_immediate)
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    if (present_mode == MVR_PRESENT_MODE_TRIPLE_BUFFER && m_surface_format.supports_mailbox)
        return VK_PRESENT_MODE_MAILBOX_KHR;
    return VK_PRESENT_MODE_FIFO_KHR;
}

void MVRender::Renderer::initialize_swapchain() {
    VkSwapchainCreateInfoKHR sc_create_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            //.oldSwapchain = nullptr, TODO: Possibly use this later down the line
            .surface = m_vk_surface,
            .minImageCount = m_surface_format.min_image_count <= 3 ? 3 : m_surface_format.min_image_count,
            .imageFormat = m_surface_format.format,
            .imageColorSpace = m_surface_format.color_space,
            .imageExtent = {
                    .width = m_surface_format.width,
                    .height = m_surface_format.height
            },
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = get_present_mode(m_initialize_params.present_mode),
            .clipped = true,
    };
    VkResult swapchain_result = vkCreateSwapchainKHR(m_vk_logical_device, &sc_create_info, VK_NULL_HANDLE, &m_vk_swapchain);

    if (swapchain_result != VK_SUCCESS) {
        const char *result_string = string_VkResult(swapchain_result);
        throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create swapchain with Vulkan error {}", result_string));
    }

    // Get swapchain images
    std::vector<VkImage> swapchain_images;
    vkGetSwapchainImagesKHR(m_vk_logical_device, m_vk_swapchain, &m_swapchain_image_count, nullptr);
    swapchain_images.resize(m_swapchain_image_count);
    vkGetSwapchainImagesKHR(m_vk_logical_device, m_vk_swapchain, &m_swapchain_image_count, swapchain_images.data());

    // Create per-swapchain resources
    for (auto image: swapchain_images) {
        // Create image view
        VkImageViewCreateInfo image_view_create_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = m_surface_format.format,
                .components = {
                        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                        .a = VK_COMPONENT_SWIZZLE_IDENTITY
                },
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                }
        };
        VkImageView image_view;
        VkResult image_view_result = vkCreateImageView(m_vk_logical_device, &image_view_create_info, nullptr, &image_view);

        if (image_view_result != VK_SUCCESS) {
            const char *string_result = string_VkResult(image_view_result);
            throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create swapchain image view, Vulkan error {}", string_result));
        }

        // Create image ready semaphore
        VkSemaphore semaphore;
        VkSemaphoreCreateInfo semaphore_create_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkResult semaphore_result = vkCreateSemaphore(m_vk_logical_device, &semaphore_create_info, nullptr, &semaphore);

        if (semaphore_result != VK_SUCCESS) {
            const char *string_result = string_VkResult(semaphore_result);
            throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create swapchain semaphore, Vulkan error {}", string_result));
        }

        // Create submit ready semaphore
        VkSemaphore semaphore2;
        VkSemaphoreCreateInfo semaphore_create_info2 = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkResult semaphore_result2 = vkCreateSemaphore(m_vk_logical_device, &semaphore_create_info2, nullptr, &semaphore2);

        if (semaphore_result2 != VK_SUCCESS) {
            const char *string_result = string_VkResult(semaphore_result2);
            throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create swapchain semaphore, Vulkan error {}", string_result));
        }

        SwapchainResources swapchain_resources = {
                .image = image,
                .image_view = image_view,
                .image_ready_semaphore = semaphore,
                .submit_ready_semaphore = semaphore2,
        };
        m_swapchain_res.push_back(swapchain_resources);
    }

    spdlog::info("Successfully created swapchain.");
}

void MVRender::Renderer::quit_swapchain() {
    // Destroy the image views
    for (auto swapchain_resource: m_swapchain_res) {
        vkDestroyImageView(m_vk_logical_device, swapchain_resource.image_view, nullptr);
        vkDestroySemaphore(m_vk_logical_device, swapchain_resource.submit_ready_semaphore, nullptr);
        vkDestroySemaphore(m_vk_logical_device, swapchain_resource.image_ready_semaphore, nullptr);
    }

    vkDestroySwapchainKHR(m_vk_logical_device, m_vk_swapchain, nullptr);
}

void MVRender::Renderer::initialize_sync() {
    VkSemaphoreTypeCreateInfo timeline_create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = m_frame_count,
    };
    VkSemaphoreCreateInfo semaphore_create_info = {
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = &timeline_create_info
    };
    VkResult result = vkCreateSemaphore(m_vk_logical_device, &semaphore_create_info, nullptr, &m_timeline_semaphore);
    if (result != VK_SUCCESS) {
        const char *string_result = string_VkResult(result);
        throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create timeline semaphore, Vulkan error {}", string_result));
    }
    spdlog::info("Create timeline semaphore.");
}

void MVRender::Renderer::quit_sync() {
    vkDestroySemaphore(m_vk_logical_device, m_timeline_semaphore, nullptr);
}

void MVRender::Renderer::initialize_frame_resources() {
    // Create command pool
    VkCommandPoolCreateInfo command_pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = m_queue_family_index,
    };
    VkResult command_pool_result = vkCreateCommandPool(m_vk_logical_device, &command_pool_create_info, nullptr, &m_command_pool);
    if (command_pool_result != VK_SUCCESS) {
        const char *string_result = string_VkResult(command_pool_result);
        throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create command pool, Vulkan error {}", string_result));
    }

    // Create per-frame resources
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkCommandBuffer command_buffers[3];
        VkCommandBufferAllocateInfo allocate_info = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool = m_command_pool,
                .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 3,
        };
        VkResult allocate_result = vkAllocateCommandBuffers(m_vk_logical_device, &allocate_info, command_buffers);
        if (allocate_result != VK_SUCCESS) {
            const char *string_result = string_VkResult(allocate_result);
            throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create command buffers, Vulkan error {}", string_result));
        }

        BufferAllocatorCreateInfo buffer_allocator_create_info = {
                .allocator = m_vma,
                .logical_device = m_vk_logical_device,
                .page_size = VRAM_PAGE_SIZE,
                .queue_family_index = m_queue_family_index,
                .device_properties = m_vk_physical_device_properties,
        };

        FrameResources res = {
                .copy_commands = command_buffers[0],
                .compute_commands = command_buffers[1],
                .draw_commands = command_buffers[2],
                .buffer_allocator = BufferAllocator(buffer_allocator_create_info),
        };

        m_frame_res.push_back(res);
    }
    spdlog::info("Created per-frame resources.");
}

void MVRender::Renderer::quit_frame_resources() {
    vkDestroyCommandPool(m_vk_logical_device, m_command_pool, nullptr);
    // Intentionally free items in the frame resource list to call their destructors
    m_frame_res.resize(0);
    spdlog::info("Freed per-frame resources.");
}

void MVRender::Renderer::initialize_vma() {
    VmaAllocatorCreateInfo allocator_create_info = {
        .physicalDevice = m_vk_physical_device,
        .device = m_vk_logical_device,
        .instance = m_vk_instance,
        .vulkanApiVersion = VK_MAKE_VERSION(1, 3, 0),
    };
    VkResult allocator_result = vmaCreateAllocator(&allocator_create_info, &m_vma);

    if (allocator_result != VK_SUCCESS) {
        const char *string_result = string_VkResult(allocator_result);
        throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to create allocator, {}", string_result));
    }

    spdlog::info("Initialized VMA");
}

void MVRender::Renderer::quit_vma() {
    vmaDestroyAllocator(m_vma);
    spdlog::info("Freed VMA");
}

void MVRender::Renderer::begin_frame() {
    // Wait for frames-in-flight to catch up
    uint64_t wait_value = m_frame_count - FRAMES_IN_FLIGHT + 1;
    VkSemaphoreWaitInfo semaphore_wait_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .semaphoreCount = 1,
        .pSemaphores = &m_timeline_semaphore,
        .pValues = &wait_value,
    };
    vkWaitSemaphores(m_vk_logical_device, &semaphore_wait_info, UINT64_MAX);

    // Reset and begin this frame's command buffers
    FrameResources *frame = &m_frame_res[m_frame_count % FRAMES_IN_FLIGHT];
    vkResetCommandBuffer(frame->compute_commands, 0);
    vkResetCommandBuffer(frame->copy_commands, 0);
    vkResetCommandBuffer(frame->draw_commands, 0);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    vkBeginCommandBuffer(frame->compute_commands, &begin_info);
    vkBeginCommandBuffer(frame->copy_commands, &begin_info);
    vkBeginCommandBuffer(frame->draw_commands, &begin_info);

    // Prepare temp buffers
    frame->buffer_allocator.begin_frame();

    // Now that we have a frame in flight, acquire the swapchain image
    vkAcquireNextImageKHR(m_vk_logical_device, m_vk_swapchain, UINT64_MAX, m_swapchain_res[m_frame_count % m_swapchain_image_count].image_ready_semaphore, nullptr, &m_current_sc_image);
}

void MVRender::Renderer::end_frame() {
    FrameResources *frame = &m_frame_res[m_frame_count % FRAMES_IN_FLIGHT];

    // TODO: Remove this garbage (this exists to pretend there is stuff drawn so it dont instantly crash)
    VkImageMemoryBarrier2 barrier{
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask = VK_ACCESS_2_NONE,
            .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .image = m_swapchain_res[m_current_sc_image].image,
            .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0, .levelCount = 1,
                    .baseArrayLayer = 0, .layerCount = 1,
            },
    };

    VkDependencyInfo depInfo{
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(frame->draw_commands, &depInfo);

    // Let temp buffer record its commands before ending
    frame->buffer_allocator.record_copy_commands(frame->copy_commands);

    // End command buffers for the frame
    vkEndCommandBuffer(frame->compute_commands);
    vkEndCommandBuffer(frame->copy_commands);
    vkEndCommandBuffer(frame->draw_commands);

    // Prepare the final frame submission
    VkCommandBuffer buffers[] = {
            frame->copy_commands,
            frame->compute_commands,
            frame->draw_commands,
    };
    uint64_t signal_values[] = {m_frame_count + 1, 1};
    VkTimelineSemaphoreSubmitInfo timelineSubmit = {
            .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
            .signalSemaphoreValueCount = 2,
            .pSignalSemaphoreValues = signal_values,
    };
    VkSemaphore signal_semaphores[] = {m_timeline_semaphore, m_swapchain_res[m_frame_count % m_swapchain_image_count].submit_ready_semaphore};
    VkPipelineStageFlags wait_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &timelineSubmit,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_swapchain_res[m_frame_count % m_swapchain_image_count].image_ready_semaphore,
        .pWaitDstStageMask = &wait_stage_mask,
        .commandBufferCount = 3,
        .pCommandBuffers = buffers,
        .signalSemaphoreCount = 2,
        .pSignalSemaphores = signal_semaphores,
    };
    VkResult queue_submit_result = vkQueueSubmit(m_vk_queue, 1, &submit_info, nullptr);

    if (queue_submit_result != VK_SUCCESS) {
        const char *string_result = string_VkResult(queue_submit_result);
        throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to submit queue, Vulkan error {}", string_result));
    }

    // Present the queue
    VkPresentInfoKHR present_info = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &m_swapchain_res[m_frame_count % m_swapchain_image_count].submit_ready_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &m_vk_swapchain,
            .pImageIndices = &m_current_sc_image,
    };
    VkResult queue_present_result = vkQueuePresentKHR(m_vk_queue, &present_info);

    if (queue_present_result != VK_SUBOPTIMAL_KHR && queue_present_result != VK_ERROR_OUT_OF_DATE_KHR && queue_present_result != VK_SUCCESS) {
        const char *string_result = string_VkResult(queue_present_result);
        throw Exception(MVR_RESULT_CRITICAL_VULKAN_ERROR, fmt::format("Failed to present queue, Vulkan error {}", string_result));
    }

    m_frame_count += 1;

    if (queue_present_result != VK_SUCCESS) {
        // TODO: Rebuild the swapchain if its out of date
    }
}
