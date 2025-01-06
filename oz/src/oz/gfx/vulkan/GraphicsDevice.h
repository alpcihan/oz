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
    // create //
    Window        createWindow(uint32_t width, uint32_t height, const char* name = "");
    CommandBuffer createCommandBuffer();
    Shader        createShader(const std::string& path, ShaderStage stage);
    RenderPass    createRenderPass(Shader                                vertexShader,
                                   Shader                                fragmentShader,
                                   Window                                window,
                                   VkPipelineVertexInputStateCreateInfo* vertexInputInfo);
    Semaphore     createSemaphore();
    Fence         createFence();
    Buffer        createBuffer(BufferType bufferType, uint64_t size, const void* data = nullptr);

    // sync //
    void waitIdle() const;
    void waitGraphicsQueueIdle() const;
    void waitFences(Fence fence, uint32_t fenceCount, bool waitAll = true) const;
    void resetFences(Fence fence, uint32_t fenceCount) const;

    // state getters //
    CommandBuffer getCurrentCommandBuffer();
    uint32_t      getCurrentImage(Window window) const;

    // window //
    bool isWindowOpen(Window window) const;
    void presentImage(Window window, uint32_t imageIndex);

    // commands //
    void beginCmd(CommandBuffer cmd, bool isSingleUse = false) const;
    void endCmd(CommandBuffer cmd) const;
    void submitCmd(CommandBuffer cmd) const;
    void beginRenderPass(CommandBuffer cmd, RenderPass renderPass, uint32_t imageIndex) const;
    void endRenderPass(CommandBuffer cmd) const;
    void draw(CommandBuffer cmd,
              uint32_t      vertexCount,
              uint32_t      instanceCount = 1,
              uint32_t      firstVertex   = 0,
              uint32_t      firstInstance = 0) const;
    void drawIndexed(CommandBuffer cmd,
                     uint32_t      indexCount,
                     uint32_t      instanceCount = 1,
                     uint32_t      firstIndex    = 0,
                     uint32_t      vertexOffset  = 0,
                     uint32_t      firstInstance = 0) const;
    void bindVertexBuffer(CommandBuffer cmd, Buffer vertexBuffer);
    void bindIndexBuffer(CommandBuffer cmd, Buffer indexBuffer);

    void copyBuffer(Buffer src, Buffer dst, uint64_t size);

    // free //
    void free(Window window) const;
    void free(Shader shader) const;
    void free(RenderPass renderPass) const;
    void free(Semaphore semaphore) const;
    void free(Fence fence) const;
    void free(CommandBuffer commandBuffer) const;
    void free(Buffer buffer) const;

  public:
    VkDevice m_device = VK_NULL_HANDLE;

  private:
    VkInstance       m_instance       = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;

    VkQueue                              m_graphicsQueue = VK_NULL_HANDLE;
    std::vector<VkQueueFamilyProperties> m_queueFamilies;
    uint32_t                             m_graphicsFamily = VK_QUEUE_FAMILY_IGNORED;

    VkCommandPool m_commandPool = VK_NULL_HANDLE; // TODO: Support multiple command pools

    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    std::vector<CommandBuffer> m_commandBuffers;
    std::vector<Fence>         m_inFlightFences;
    std::vector<Semaphore>     m_imageAvailableSemaphores;
    std::vector<Semaphore>     m_renderFinishedSemaphores;

    uint32_t m_currentFrame = 0;
};

} // namespace oz::gfx::vk