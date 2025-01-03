#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

struct IVkData {
    virtual ~IVkData()                   = default;
    virtual void free(VkDevice vkDevice) = 0;
};

struct ShaderData final : IVkData {
    ShaderStage stage;

    VkShaderModule                  vkShaderModule                  = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};

    void free(VkDevice vkDevice) override { vkDestroyShaderModule(vkDevice, vkShaderModule, nullptr); }
};

struct RenderPassData final : IVkData {
    VkRenderPass               vkRenderPass       = VK_NULL_HANDLE;
    VkPipelineLayout           vkPipelineLayout   = VK_NULL_HANDLE;
    VkPipeline                 vkGraphicsPipeline = VK_NULL_HANDLE;
    VkExtent2D                 vkExtent           = {};
    std::vector<VkFramebuffer> vkFrameBuffers;

    void free(VkDevice vkDevice) override {
        vkDestroyPipeline(vkDevice, vkGraphicsPipeline, nullptr);
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
        vkDestroyRenderPass(vkDevice, vkRenderPass, nullptr);

        for (auto framebuffer : vkFrameBuffers) {
            vkDestroyFramebuffer(vkDevice, framebuffer, nullptr);
        }
    }
};

struct SemaphoreData final : IVkData {
    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    // TODO: only vkSemaphore is supported

    void free(VkDevice vkDevice) override { vkDestroySemaphore(vkDevice, vkSemaphore, nullptr); }
};

struct FenceData final : IVkData {
    VkFence vkFence = VK_NULL_HANDLE;
    // TODO: only vkFence is supported
    void free(VkDevice vkDevice) override { vkDestroyFence(vkDevice, vkFence, nullptr); }
};

struct WindowData final : IVkData {
    GLFWwindow*  vkWindow  = nullptr;
    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;

    VkSwapchainKHR           vkSwapChain            = VK_NULL_HANDLE;
    VkExtent2D               vkSwapChainExtent      = {};
    VkFormat                 vkSwapChainImageFormat = VK_FORMAT_UNDEFINED;
    std::vector<VkImage>     vkSwapChainImages;
    std::vector<VkImageView> vkSwapChainImageViews;

    VkQueue vkPresentQueue = VK_NULL_HANDLE;

    VkInstance vkInstance = VK_NULL_HANDLE; // referenced to used on free

    void free(VkDevice vkDevice) override {
        // swap chain
        vkDestroySwapchainKHR(vkDevice, vkSwapChain, nullptr);

        // image views
        for (auto imageView : vkSwapChainImageViews) {
            vkDestroyImageView(vkDevice, imageView, nullptr);
        }

        // images
        // for (auto image : window->vkSwapChainImages) {
        //    vkDestroyImage(vkDevice, image, nullptr);
        // }

        // surface
        if (vkSurface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(vkInstance, vkSurface, nullptr);
        }

        // window
        if (vkWindow != nullptr) {
            glfwDestroyWindow(vkWindow);
        }
    }
};

struct CommandBufferData final : IVkData {
    VkCommandBuffer vkCommandBuffer = VK_NULL_HANDLE;

    void free(VkDevice vkDevice) override {}
};

#define OZ_CREATE_VK_DATA(TYPE) new TYPE##Data

#define OZ_FREE_VK_DATA(vkData) \
    if (vkData) {               \
        vkData->free(m_device); \
        delete vkData;          \
        vkData = nullptr;       \
    }

} // namespace oz::gfx::vk