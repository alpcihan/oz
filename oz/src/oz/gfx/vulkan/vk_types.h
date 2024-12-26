#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

#define VK_TYPE(NAME)           \
    struct NAME##Data;          \
    typedef NAME##Data* NAME;


VK_TYPE(Shader);


enum class ShaderStage: uint8_t {
    Vertex,
    Fragment,
    Compute
};

} // namespace oz::gfx::vk