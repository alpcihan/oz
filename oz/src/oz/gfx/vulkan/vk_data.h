#pragma once

#include "oz/gfx/vulkan/vk_base.h"
#include "oz/gfx/vulkan/vk_types.h"

namespace oz::gfx::vk {

struct ShaderData {
    ShaderStage stage;

    VkShaderModule vkShaderModule;
    VkPipelineShaderStageCreateInfo stageInfo;
};

struct RenderPassData {
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
};

} // namespace oz::gfx::vk