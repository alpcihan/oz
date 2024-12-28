#include "oz/core/file/file.h"
#include "oz/gfx/vulkan/GraphicsDevice.h"
#include "oz/gfx/vulkan/vk_data.h" // TODO: remove
#include "oz/gfx/vulkan/vk_utils.h"

int main() {
    const int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame          = 0;

    // init
    std::unique_ptr<oz::gfx::vk::GraphicsDevice> device = std::make_unique<oz::gfx::vk::GraphicsDevice>(true);
    GLFWwindow* window                              = device->createWindow(800, 600, "oz");

    oz::gfx::vk::Shader vertShader = device->createShader("default_vert.spv", oz::gfx::vk::ShaderStage::Vertex);
    oz::gfx::vk::Shader fragShader = device->createShader("default_frag.spv", oz::gfx::vk::ShaderStage::Fragment);

    oz::gfx::vk::RenderPass renderPass = device->createRenderPass(vertShader, fragShader);
    device->free(vertShader);
    device->free(fragShader);

    std::vector<oz::gfx::vk::CommandBuffer> commandBuffers;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        commandBuffers.emplace_back(device->createCommandBuffer());

    std::vector<oz::gfx::vk::Semaphore> imageAvailableSemaphores;
    std::vector<oz::gfx::vk::Semaphore> renderFinishedSemaphores;
    std::vector<oz::gfx::vk::Fence> inFlightFences;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        imageAvailableSemaphores.push_back(device->createSemaphore());
        renderFinishedSemaphores.push_back(device->createSemaphore());
        inFlightFences.push_back(device->createFence());
    }

    // main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        device->waitFences(inFlightFences[currentFrame], 1);
        device->resetFences(inFlightFences[currentFrame], 1);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(device->getVkDevice(), device->getVkSwapchain(), UINT64_MAX,
                              imageAvailableSemaphores[currentFrame]->vkSemaphore, VK_NULL_HANDLE, &imageIndex);

        auto& cmd = commandBuffers[currentFrame];
        cmd.begin();
        cmd.beginRenderPass(renderPass, imageIndex);
        cmd.draw(3);
        cmd.endRenderPass();
        cmd.end();

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[]      = {imageAvailableSemaphores[currentFrame]->vkSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount     = 1;
        submitInfo.pWaitSemaphores        = waitSemaphores;
        submitInfo.pWaitDstStageMask      = waitStages;
        submitInfo.commandBufferCount     = 1;
        submitInfo.pCommandBuffers        = &commandBuffers[currentFrame].m_vkCommandBuffer;

        VkSemaphore signalSemaphores[]  = {renderFinishedSemaphores[currentFrame]->vkSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;

        if (vkQueueSubmit(device->getVkGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]->vkFence) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;

        VkSwapchainKHR swapChains[] = {device->getVkSwapchain()};
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;
        presentInfo.pImageIndices   = &imageIndex;
        presentInfo.pResults        = nullptr; // optional

        vkQueuePresentKHR(device->getVkPresentQueue(), &presentInfo);

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    vkDeviceWaitIdle(device->getVkDevice());

    // cleanup
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        device->free(renderFinishedSemaphores[i]);
        device->free(imageAvailableSemaphores[i]);
        device->free(inFlightFences[i]);
    }

    device->free(renderPass);

    return 0;
}