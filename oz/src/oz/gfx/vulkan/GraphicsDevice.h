#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

class GraphicsDevice final {
  public:
    GraphicsDevice(const bool enableValidationLayers = false);

    GraphicsDevice(const GraphicsDevice &)            = delete;
    GraphicsDevice &operator=(const GraphicsDevice &) = delete;

    GraphicsDevice(GraphicsDevice &&)            = delete;
    GraphicsDevice &operator=(GraphicsDevice &&) = delete;

    ~GraphicsDevice();

  public:
    GLFWwindow *createWindow(const uint32_t width, const uint32_t height, const char *name = ""); // TODO: move to a window class
    VkCommandBuffer createCommandBuffer();
    Shader createShader(const std::string &path, ShaderStage stage);
    RenderPass createRenderPass(Shader vertexShader, Shader fragmentShader);

    VkDevice getVkDevice() const { return m_device; }
    VkQueue getVkGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getVkPresentQueue() const { return m_presentQueue; }
    VkSurfaceKHR getVkSurface() const { return m_surface; }

    VkSwapchainKHR getVkSwapchain() const { return m_swapChain; }
    VkFormat getVkSwapchainImageFormat() const { return m_swapChainImageFormat; }
    const std::vector<VkImageView> &getSwapChainImageViews() const { return m_swapChainImageViews; }
    const VkExtent2D &getVkSwapchainExtent() const { return m_swapChainExtent; }
    const std::vector<VkImage> &getSwapChainImages() const { return m_swapChainImages; }
    // const std::vector<VkFramebuffer> &getSwapChainFrameBuffers() const { return m_swapChainFramebuffers; }

    void free(Shader shader);
    void free(RenderPass renderPass);

  private:
    VkInstance m_instance             = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device                 = VK_NULL_HANDLE;
    VkCommandPool m_commandPool       = VK_NULL_HANDLE; // TODO: Support multiple command pools

    VkSurfaceKHR m_surface     = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    // std::vector<VkFramebuffer> m_swapChainFramebuffers;

    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    std::vector<VkQueueFamilyProperties> m_queueFamilies;
    VkQueue m_presentQueue    = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue   = VK_NULL_HANDLE;
    uint32_t m_graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
    uint32_t m_presentFamily  = VK_QUEUE_FAMILY_IGNORED;

    // window (TODO: move to a window class)
    GLFWwindow *m_window = nullptr;

  private:
    VkCommandPool _createCommandPool();
};

} // namespace oz::gfx::vk