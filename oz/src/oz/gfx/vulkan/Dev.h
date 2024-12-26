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

    std::vector<gfx::vk::CommandBuffer> m_commandBuffers;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

  private:
    void _init() {
        m_device = std::make_unique<gfx::vk::GraphicsDevice>(true);
        m_window = m_device->createWindow(800, 600, "oz");

        gfx::vk::Shader vertShader = m_device->createShader("default_vert.spv", gfx::vk::ShaderStage::Vertex);
        gfx::vk::Shader fragShader = m_device->createShader("default_frag.spv", gfx::vk::ShaderStage::Fragment);

        m_renderPass = m_device->createRenderPass(vertShader, fragShader);

        m_device->free(vertShader);
        m_device->free(fragShader);

        for (int i = 0; i < m_MAX_FRAMES_IN_FLIGHT; i++)
            m_commandBuffers.push_back(m_device->createCommandBuffer());

        _createSyncObjects();
    }

    void _recordCommandBuffer(gfx::vk::CommandBuffer& cmd, uint32_t imageIndex) {
        cmd.begin();
        cmd.beginRenderPass(m_renderPass, imageIndex);
        cmd.draw(3);
        cmd.endRenderPass();
        cmd.end();
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

        auto& cmd = m_commandBuffers[m_currentFrame];
        cmd.begin();
        cmd.beginRenderPass(m_renderPass, imageIndex);
        cmd.draw(3);
        cmd.endRenderPass();
        cmd.end();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]      = {m_imageAvailableSemaphores[m_currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount     = 1;
        submitInfo.pWaitSemaphores        = waitSemaphores;
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = 1;
        submitInfo.pCommandBuffers        = &m_commandBuffers[m_currentFrame].m_vkCommandBuffer;

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
    }
};

} // namespace oz