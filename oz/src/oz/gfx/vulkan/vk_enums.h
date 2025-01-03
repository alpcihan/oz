#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

enum class ShaderStage : uint8_t { Vertex = 0x00000001, Fragment = 0x00000010, Compute = 0x00000020 };

} // namespace oz::gfx::vk