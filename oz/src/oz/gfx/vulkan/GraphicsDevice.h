#pragma once

#include "oz/gfx/vulkan/CommandBuffer.h"
#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

class GraphicsDevice final {
  public:
    GraphicsDevice(const bool enableValidationLayers = false);

    GraphicsDevice(const GraphicsDevice&)            = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;

    GraphicsDevice(GraphicsDevice&&)            = delete;
    GraphicsDevice& operator=(GraphicsDevice&&) = delete;

    ~GraphicsDevice();

  public:
    GLFWwindow* createWindow(const uint32_t width,
                             const uint32_t height,
                             const char* name = ""); // TODO: move to a window class
    CommandBuffer createCommandBuffer();
    Shader createShader(const std::string& path, ShaderStage stage);
    RenderPass createRenderPass(Shader vertexShader, Shader fragmentShader);
    Semaphore createSemaphore();
    Fence createFence();

    void waitFences(Fence fence, uint32_t fenceCount, bool waitAll = true);
    void resetFences(Fence fence, uint32_t fenceCount);

    VkDevice getVkDevice() const { return m_device; }
    VkQueue getVkGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue getVkPresentQueue() const { return m_presentQueue; }

    VkSwapchainKHR getVkSwapchain() const { return m_swapChain; }

    void free(Shader shader);
    void free(RenderPass renderPass);
    void free(Semaphore semaphore);
    void free(Fence fence);

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

    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    std::vector<VkQueueFamilyProperties> m_queueFamilies;
    VkQueue m_presentQueue    = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue   = VK_NULL_HANDLE;
    uint32_t m_graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
    uint32_t m_presentFamily  = VK_QUEUE_FAMILY_IGNORED;

    GLFWwindow* m_window = nullptr; // window (TODO: move to a window class)

  private:
    VkCommandPool _createCommandPool();
};

} // namespace oz::gfx::vk