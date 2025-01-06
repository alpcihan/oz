#pragma once

#include "oz/gfx/vulkan/vk_common.h"

namespace oz::gfx::vk {

#define OZ_VK_OBJECT(NAME) \
    struct NAME##Object;   \
    typedef NAME##Object* NAME;

OZ_VK_OBJECT(Window);
OZ_VK_OBJECT(Shader);
OZ_VK_OBJECT(RenderPass);
OZ_VK_OBJECT(Fence);
OZ_VK_OBJECT(Semaphore);
OZ_VK_OBJECT(CommandBuffer);
OZ_VK_OBJECT(Buffer);

} // namespace oz::gfx::vk