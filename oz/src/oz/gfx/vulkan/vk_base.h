#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "oz/base.h"

namespace oz::gfx::vk {

#define VK_TYPE(NAME)  \
    struct NAME##Data; \
    typedef NAME##Data* NAME;

VK_TYPE(Shader);
VK_TYPE(RenderPass);
VK_TYPE(Fence);
VK_TYPE(Semaphore);

enum class ShaderStage : uint8_t { Vertex = 0x00000001, Fragment = 0x00000010, Compute = 0x00000020 };

} // namespace oz::gfx::vk