#pragma once

#include "oz/gfx/vulkan/vk_base.h"
#include "oz/gfx/vulkan/vk_types.h"

namespace oz::gfx::vk {

struct ShaderData {
    ShaderStage stage;

    VkShaderModule vkShaderModule;
    VkPipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo;
};

struct RenderPassData {
    VkRenderPass vkRenderPass;
    VkPipelineLayout vkPipelineLayout;
    VkPipeline vkGraphicsPipeline;
};

} // namespace oz::gfx::vk