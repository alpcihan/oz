#include "oz/gfx/vulkan/CommandBuffer.h"
#include "oz/gfx/vulkan/vk_utils.h"
#include "oz/gfx/vulkan/vk_data.h"

namespace oz::gfx::vk {

void CommandBuffer::begin() {
    vkResetCommandBuffer(m_vkCommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = 0;       // optional
    beginInfo.pInheritanceInfo = nullptr; // optional

    assert(vkBeginCommandBuffer(m_vkCommandBuffer, &beginInfo) == VK_SUCCESS);
}

void CommandBuffer::end() { assert(vkEndCommandBuffer(m_vkCommandBuffer) == VK_SUCCESS); }

void CommandBuffer::beginRenderPass(RenderPass renderPass, uint32_t imageIndex) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass->vkRenderPass;
    renderPassInfo.framebuffer       = renderPass->vkFrameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = renderPass->vkExtent;
    VkClearValue clearColor          = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount   = 1;
    renderPassInfo.pClearValues      = &clearColor;

    vkCmdBeginRenderPass(m_vkCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass->vkGraphicsPipeline);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(renderPass->vkExtent.width);
    viewport.height   = static_cast<float>(renderPass->vkExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_vkCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = renderPass->vkExtent;
    vkCmdSetScissor(m_vkCommandBuffer, 0, 1, &scissor);
}

void CommandBuffer::endRenderPass() { vkCmdEndRenderPass(m_vkCommandBuffer); }

void CommandBuffer::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(m_vkCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

CommandBuffer::CommandBuffer(VkDevice vkDevice, VkCommandPool vkCommandPool) {
    assert(ivkAllocateCommandBuffers(vkDevice, vkCommandPool, 1, &m_vkCommandBuffer) == VK_SUCCESS);
}

} // namespace oz::gfx::vk