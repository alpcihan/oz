#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

struct ShaderData final {
    ShaderStage stage;

    VkShaderModule vkShaderModule;
    VkPipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo;
};

struct RenderPassData final {
    VkRenderPass vkRenderPass;
    VkPipelineLayout vkPipelineLayout;
    VkPipeline vkGraphicsPipeline;
};

} // namespace oz::gfx::vk