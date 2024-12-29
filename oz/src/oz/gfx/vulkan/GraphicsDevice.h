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
    Window createWindow(const uint32_t width,
                             const uint32_t height,
                             const char* name = "");
    CommandBuffer createCommandBuffer();
    Shader createShader(const std::string& path, ShaderStage stage);
    RenderPass createRenderPass(Shader vertexShader, Shader fragmentShader, Window window);
    Semaphore createSemaphore();
    Fence createFence();

    void waitFences(Fence fence, uint32_t fenceCount, bool waitAll = true);
    void resetFences(Fence fence, uint32_t fenceCount);

    VkDevice getVkDevice() const { return m_device; }
    VkQueue getVkGraphicsQueue() const { return m_graphicsQueue; }

    void free(Window window);
    void free(Shader shader);
    void free(RenderPass renderPass);
    void free(Semaphore semaphore);
    void free(Fence fence);

  private:
    VkInstance m_instance             = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device                 = VK_NULL_HANDLE;
    VkCommandPool m_commandPool       = VK_NULL_HANDLE; // TODO: Support multiple command pools

    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    std::vector<VkQueueFamilyProperties> m_queueFamilies;
    VkQueue m_graphicsQueue   = VK_NULL_HANDLE;
    uint32_t m_graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
    
  private:
    VkCommandPool _createCommandPool();
};

} // namespace oz::gfx::vk