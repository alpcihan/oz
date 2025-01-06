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

    OZ_CHAINED_SETTER(setOffset, size_t, offset)
    OZ_CHAINED_SETTER(setFormat, Format, format)
};

struct VertexLayout final {
    uint32_t                           vertexSize;
    std::vector<VertexLayoutAttribute> vertexLayoutAttributes;

    VertexLayout(uint32_t _vertexSize, std::vector<VertexLayoutAttribute> const& _vertexLayoutAttributes)
        : vertexSize(_vertexSize), vertexLayoutAttributes(_vertexLayoutAttributes){}
};

}; // namespace oz::gfx::vk