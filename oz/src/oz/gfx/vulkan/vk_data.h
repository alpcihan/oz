#pragma once

#include "oz/gfx/vulkan/vk_base.h"
#include "oz/gfx/vulkan/vk_types.h"

namespace oz::gfx::vk {

struct ShaderData {
    VkShaderModule vkShaderModule;
    ShaderStage stage;
};

} // namespace oz::gfx::vk