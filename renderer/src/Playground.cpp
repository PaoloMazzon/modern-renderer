#include <volk.h>
#include <spdlog/spdlog.h>
#include "render/Logging.hpp"
#include "render/Renderer.hpp"
#include "render/Util.hpp"

#ifdef BUILD_PLAYGROUND
struct PushConstants {
    float time;
};

void MVRender::Renderer::initialize_playground() {
    VkPushConstantRange push_constant_range = {
        .stageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        .offset = 0,
        .size = sizeof(PushConstants),
    };
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = nullptr,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range
    }; // TODO: Come back to this when we have a descriptor abstraction, otherwise this is hell
    VkResult result = vkCreatePipelineLayout(m_vk_logical_device, &pipeline_layout_create_info, nullptr, &m_playground_compute_pipe_layout);
    resolve_vulkan_error(result, true, "Failed to create pipeline layout");

    std::vector<uint8_t> shader_code = read_buffer("assets/test_shader.spv");
    VkShaderModuleCreateInfo compute_shader_module_create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .flags = 0,
        .codeSize = shader_code.size(),
        .pCode = reinterpret_cast<uint32_t*>(shader_code.data())
    };

    VkShaderModule compute_shader_module;
    result = vkCreateShaderModule(m_vk_logical_device, &compute_shader_module_create_info, nullptr, &compute_shader_module);
    resolve_vulkan_error(result, true, "Failed to create shader module");

    VkPipelineShaderStageCreateInfo compute_shader_stage_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compute_shader_module,
        .pName = "compute shader",
    };

    VkComputePipelineCreateInfo pipeline_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = compute_shader_stage_create_info,
        .layout = m_playground_compute_pipe_layout,
        .basePipelineHandle = nullptr,
        .basePipelineIndex = 0,
    };
    result = vkCreateComputePipelines(m_vk_logical_device, nullptr, 1, &pipeline_create_info, nullptr, &m_playground_compute_pipe);
    resolve_vulkan_error(result, true, "Failed to create pipeline");
    vkDestroyShaderModule(m_vk_logical_device, compute_shader_module, nullptr);

    spdlog::info("Created testing compute shader.");
}
void MVRender::Renderer::quit_playground() {
    vkDestroyPipelineLayout(m_vk_logical_device, m_playground_compute_pipe_layout, nullptr);
    vkDestroyPipeline(m_vk_logical_device, m_playground_compute_pipe, nullptr);
}
void MVRender::Renderer::update_playground() {
    // todo this shit (ts)
}
#endif // BUILD_PLAYGROUND