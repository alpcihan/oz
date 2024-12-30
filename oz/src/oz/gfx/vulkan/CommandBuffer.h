#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

class CommandBuffer final {
    friend class GraphicsDevice;

  public:
    void begin();
    void end();

    void beginRenderPass(RenderPass renderPass, uint32_t imageIndex);
    void endRenderPass();

    void draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0);

  protected:
    CommandBuffer(VkDevice vkDevice, VkCommandPool vkCommandPool);

  public:
    VkCommandBuffer m_vkCommandBuffer;
};

} // namespace oz::gfx::vk