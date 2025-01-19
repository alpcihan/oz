#pragma once

#include "oz/gfx/vulkan/vk_common.h"

namespace oz::gfx::vk {

#define OZ_CHAINED_SETTER(FUNCNAME, TYPE, NAME) \
    auto& FUNCNAME(TYPE _##NAME) {              \
        NAME = _##NAME;                         \
        return *this;                           \
    }

struct VertexLayoutAttribute final {
    size_t offset;
    Format format;

    VertexLayoutAttribute(size_t _offset, Format _format) : offset(_offset), format(_format) {}
};

struct VertexLayout final {
    uint32_t                           vertexSize;
    std::vector<VertexLayoutAttribute> vertexLayoutAttributes;

    VertexLayout(uint32_t _vertexSize, std::vector<VertexLayoutAttribute> const& _vertexLayoutAttributes)
        : vertexSize(_vertexSize), vertexLayoutAttributes(_vertexLayoutAttributes) {}
};

struct SetLayoutBinding {
    BindingType type;

    SetLayoutBinding(BindingType _type) : type(_type) {}
};

struct SetLayout {
    std::vector<SetLayoutBinding> bindings;

    SetLayout(std::vector<SetLayoutBinding> const& _bindings)
        : bindings(_bindings) {}
};

}; // namespace oz::gfx::vk