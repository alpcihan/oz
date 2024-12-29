#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

struct ShaderData final {
    ShaderStage stage;

    VkShaderModule vkShaderModule                                   = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo vkPipelineShaderStageCreateInfo = {};
};

struct RenderPassData final {
    VkRenderPass vkRenderPass         = VK_NULL_HANDLE;
    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
    VkPipeline vkGraphicsPipeline     = VK_NULL_HANDLE;
    VkExtent2D vkExtent               = {};
    std::vector<VkFramebuffer> vkFrameBuffers;
};

struct SemaphoreData final {
    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    // TODO: only vkSemaphore is supported
};

struct FenceData final {
    VkFence vkFence = VK_NULL_HANDLE;
    // TODO: only vkFence is supported
};

struct WindowData final {
    GLFWwindow* vkWindow   = nullptr;
    VkSurfaceKHR vkSurface = VK_NULL_HANDLE;

    VkSwapchainKHR vkSwapChain      = VK_NULL_HANDLE;
    VkExtent2D vkSwapChainExtent    = {};
    VkFormat vkSwapChainImageFormat = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> vkSwapChainImages;
    std::vector<VkImageView> vkSwapChainImageViews;

    VkQueue vkPresentQueue = VK_NULL_HANDLE;
};

} // namespace oz::gfx::vk