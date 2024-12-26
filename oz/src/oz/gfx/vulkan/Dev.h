#pragma once

#include "oz/core/file/file.h"
#include "oz/gfx/vulkan/GraphicsDevice.h"
#include "oz/gfx/vulkan/vk_data.h" // TODO: remove
#include "oz/gfx/vulkan/vk_utils.h"

namespace oz {

class DevApp {
  public:
    void run() {
        _init();
        _mainLoop();
        _cleanup();
    }

  private:
    const int m_MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t m_currentFrame          = 0;

    std::unique_ptr<gfx::vk::GraphicsDevice> m_device;

    GLFWwindow* m_window;

    gfx::vk::RenderPass m_renderPass;

    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    std::vector<VkCommandBuffer> m_commandBuffers;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

  private:
    void _init() {
        m_device = std::make_unique<gfx::vk::GraphicsDevice>(true);
        m_window   = m_device->createWindow(800, 600, "oz");

        gfx::vk::Shader vertShader = m_device->createShader("default_vert.spv", gfx::vk::ShaderStage::Vertex);
        gfx::vk::Shader fragShader = m_device->createShader("default_frag.spv", gfx::vk::ShaderStage::Fragment);

        m_renderPass = m_device->createRenderPass(vertShader, fragShader);

        // free the shader modules
        m_device->free(vertShader);
        m_device->free(fragShader);

        _createFramebuffers();

        m_commandBuffers.resize(m_MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++)
            m_commandBuffers[i] = m_device->createCommandBuffer();

        _createSyncObjects();
    }

    void _createFramebuffers() {
        m_swapChainFramebuffers.resize(m_device->getSwapChainImageViews().size());

        for (size_t i = 0; i < m_device->getSwapChainImageViews().size(); i++) {
            VkResult result = gfx::vk::ivkCreateFramebuffer(
                m_device->getVkDevice(), m_renderPass->vkRenderPass, m_device->getVkSwapchainExtent(),
                m_device->getSwapChainImageViews()[i], &m_swapChainFramebuffers[i]);

            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void _recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags            = 0;       // optional
        beginInfo.pInheritanceInfo = nullptr; // optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass        = m_renderPass->vkRenderPass;
        renderPassInfo.framebuffer       = m_swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_device->getVkSwapchainExtent();
        VkClearValue clearColor          = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount   = 1;
        renderPassInfo.pClearValues      = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_renderPass->vkGraphicsPipeline);

        VkViewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(m_device->getVkSwapchainExtent().width);
        viewport.height   = static_cast<float>(m_device->getVkSwapchainExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = m_device->getVkSwapchainExtent();
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void _mainLoop() {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
            _drawFrame();
        }

        vkDeviceWaitIdle(m_device->getVkDevice());
    }

    void _drawFrame() {
        vkWaitForFences(m_device->getVkDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(m_device->getVkDevice(), 1, &m_inFlightFences[m_currentFrame]);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(m_device->getVkDevice(), m_device->getVkSwapchain(), UINT64_MAX,
                              m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

       
        _recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]      = {m_imageAvailableSemaphores[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount     = 1;
        submitInfo.pWaitSemaphores        = waitSemaphores;
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = 1;
        submitInfo.pCommandBuffers        = &m_commandBuffers[m_currentFrame];

        VkSemaphore signalSemaphores[]  = {m_renderFinishedSemaphores[m_currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;

        if (vkQueueSubmit(m_device->getVkGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;

        VkSwapchainKHR swapChains[] = {m_device->getVkSwapchain()};
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;
        presentInfo.pImageIndices   = &imageIndex;
        presentInfo.pResults        = nullptr; // optional

        vkQueuePresentKHR(m_device->getVkPresentQueue(), &presentInfo);

        m_currentFrame = (m_currentFrame + 1) % m_MAX_FRAMES_IN_FLIGHT;
    }

    void _createSyncObjects() {
        m_imageAvailableSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(m_MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.resize(m_MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_device->getVkDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) !=
                    VK_SUCCESS ||
                vkCreateSemaphore(m_device->getVkDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) !=
                    VK_SUCCESS ||
                vkCreateFence(m_device->getVkDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void _cleanup() {
        // synchronization objects
        for (size_t i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(m_device->getVkDevice(), m_renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(m_device->getVkDevice(), m_imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_device->getVkDevice(), m_inFlightFences[i], nullptr);
        }

        // renderpass
        m_device->free(m_renderPass);

        // framebuffers
        for (auto framebuffer : m_swapChainFramebuffers) {
            vkDestroyFramebuffer(m_device->getVkDevice(), framebuffer, nullptr);
        }
    }
};

} // namespace oz