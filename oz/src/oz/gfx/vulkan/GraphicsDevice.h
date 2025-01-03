#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

class GraphicsDevice final {
  public:
    GraphicsDevice(const bool enableValidationLayers = false);

    GraphicsDevice(const GraphicsDevice&)            = delete;
    GraphicsDevice& operator=(const GraphicsDevice&) = delete;
    GraphicsDevice(GraphicsDevice&&)                 = delete;
    GraphicsDevice& operator=(GraphicsDevice&&)      = delete;

    ~GraphicsDevice();

  public:
    // create
    Window        createWindow(uint32_t width, uint32_t height, const char* name = "");
    CommandBuffer createCommandBuffer();
    Shader        createShader(const std::string& path, ShaderStage stage);
    RenderPass    createRenderPass(Shader vertexShader, Shader fragmentShader, Window window);
    Semaphore     createSemaphore();
    Fence         createFence();

    // sync
    void waitIdle() const;
    void waitFences(Fence fence, uint32_t fenceCount, bool waitAll = true) const;
    void resetFences(Fence fence, uint32_t fenceCount) const;

    // state getters
    CommandBuffer getCurrentCommandBuffer();
    uint32_t      getCurrentImage(Window window) const;

    // window
    bool isWindowOpen(Window window) const;
    void presentImage(Window window, uint32_t imageIndex);

    // commands
    void beginCmd(CommandBuffer cmd) const;
    void endCmd(CommandBuffer cmd) const;
    void submitCmd(CommandBuffer cmd) const;
    void beginRenderPass(CommandBuffer cmd, RenderPass renderPass, uint32_t imageIndex) const;
    void endRenderPass(CommandBuffer cmd) const;
    void draw(CommandBuffer cmd,
              uint32_t      vertexCount,
              uint32_t      instanceCount = 1,
              uint32_t      firstVertex   = 0,
              uint32_t      firstInstance = 0) const;

    // free
    void free(Window window) const;
    void free(Shader shader) const;
    void free(RenderPass renderPass) const;
    void free(Semaphore semaphore) const;
    void free(Fence fence) const;
    void free(CommandBuffer commandBuffer) const;

  private:
    VkInstance       m_instance       = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice         m_device         = VK_NULL_HANDLE;
    VkCommandPool    m_commandPool    = VK_NULL_HANDLE; // TODO: Support multiple command pools

    std::vector<VkQueueFamilyProperties> m_queueFamilies;
    VkQueue                              m_graphicsQueue  = VK_NULL_HANDLE;
    uint32_t                             m_graphicsFamily = VK_QUEUE_FAMILY_IGNORED;

    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    std::vector<CommandBuffer> m_commandBuffers;
    std::vector<Fence>         m_inFlightFences;
    std::vector<Semaphore>     m_imageAvailableSemaphores;
    std::vector<Semaphore>     m_renderFinishedSemaphores;

    uint32_t m_currentFrame = 0;

  private:
    VkCommandPool _createCommandPool();
};

} // namespace oz::gfx::vk