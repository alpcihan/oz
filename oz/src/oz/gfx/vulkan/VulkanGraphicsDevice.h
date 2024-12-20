#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx {

class VulkanGraphicsDevice final {
  public:
    VulkanGraphicsDevice(const bool enableValidationLayers = false);

    VulkanGraphicsDevice(const VulkanGraphicsDevice &)            = delete;
    VulkanGraphicsDevice &operator=(const VulkanGraphicsDevice &) = delete;

    VulkanGraphicsDevice(VulkanGraphicsDevice &&)            = delete;
    VulkanGraphicsDevice &operator=(VulkanGraphicsDevice &&) = delete;

    ~VulkanGraphicsDevice();

    // TODO: move to a window class
    GLFWwindow *createWindow(const uint32_t width, const uint32_t height, const char *name = "");
    VkCommandPool createCommandPool();
    VkCommandBuffer createCommandBuffer(VkCommandPool commandPool);

    VkDevice getVkDevice() const { return m_device; }
    uint32_t getGraphicsFamily() const { return m_graphicsFamily; }
    uint32_t getPresentFamily() const { return m_presentFamily; }
    VkQueue getVkGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getVkPresentQueue() const { return m_presentQueue; }
    VkSurfaceKHR getVkSurface() const { return m_surface; }
    
    VkSwapchainKHR getVkSwapchain() const { return m_swapChain; }
    VkFormat getVkSwapchainImageFormat() const { return m_swapChainImageFormat; }
    const std::vector<VkImageView>& getSwapChainImageViews() const {return m_swapChainImageViews; }
    const VkExtent2D &getVkSwapchainExtent() const { return m_swapChainExtent; }
    const std::vector<VkImage> &getSwapChainImages() const { return m_swapChainImages; }

  private:
    VkInstance m_instance             = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device                 = VK_NULL_HANDLE;

    VkSurfaceKHR m_surface     = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    std::vector<VkQueueFamilyProperties> m_queueFamilies;
    VkQueue m_presentQueue    = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue   = VK_NULL_HANDLE;
    uint32_t m_graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
    uint32_t m_presentFamily  = VK_QUEUE_FAMILY_IGNORED;

    // window (TODO: move to a window class)
    GLFWwindow *m_window = nullptr;
};

} // namespace oz::gfx