#include "oz/gfx/vulkan/GraphicsDevice.h"
#include "oz/core/file/file.h"
#include "oz/gfx/vulkan/vk_data.h"
#include "oz/gfx/vulkan/vk_utils.h"

namespace oz::gfx::vk {

namespace {
static constexpr int FRAMES_IN_FLIGHT = 2;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

} // namespace

GraphicsDevice::GraphicsDevice(const bool enableValidationLayers) {
    // glfw
    glfwInit();
    uint32_t extensionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    const std::vector<const char*> requiredExtensions = ivkPopulateExtensions();
    const std::vector<const char*> requiredInstanceExtensions =
        ivkPopulateInstanceExtensions(extensions, extensionCount, enableValidationLayers);
    const std::vector<const char*> layers = ivkPopulateLayers(enableValidationLayers);

    assert(ivkAreLayersSupported(layers));

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = enableValidationLayers
                                                             ? ivkPopulateDebugMessengerCreateInfo(debugCallback)
                                                             : VkDebugUtilsMessengerCreateInfoEXT{};

    VkResult result;

    // create instance
    result = ivkCreateInstance(static_cast<uint32_t>(requiredInstanceExtensions.size()),
                               requiredInstanceExtensions.data(), static_cast<uint32_t>(layers.size()), layers.data(),
                               enableValidationLayers ? &debugCreateInfo : nullptr, &m_instance);
    assert(result == VK_SUCCESS);

    // create debug messenger
    if (enableValidationLayers) {
        result = ivkCreateDebugMessenger(m_instance, debugCreateInfo, &m_debugMessenger);
        assert(result == VK_SUCCESS);
    }

    // pick physical device
    ivkPickPhysicalDevice(m_instance, requiredExtensions, &m_physicalDevice, &m_queueFamilies, &m_graphicsFamily);
    assert(m_physicalDevice != VK_NULL_HANDLE);

    // create logical device (TODO: support multiple queue families)
    result = ivkCreateLogicalDevice(m_physicalDevice, {m_graphicsFamily}, requiredExtensions, layers, &m_device);
    assert(result == VK_SUCCESS);

    // get device queues
    vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);

    // create a command pool
    m_commandPool = _createCommandPool();

    // current frame
    m_currentFrame = 0;

    // command buffers
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
        m_commandBuffers.emplace_back(std::move(createCommandBuffer()));

    // synchronization
    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        m_imageAvailableSemaphores.push_back(createSemaphore());
        m_renderFinishedSemaphores.push_back(createSemaphore());
        m_inFlightFences.push_back(createFence());
    }
}

GraphicsDevice::~GraphicsDevice() {
    // command pool
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;

    // synchronization
    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        free(m_renderFinishedSemaphores[i]);
        free(m_imageAvailableSemaphores[i]);
        free(m_inFlightFences[i]);
    }
    m_renderFinishedSemaphores.clear();
    m_imageAvailableSemaphores.clear();
    m_inFlightFences.clear();

    // device
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;

    // debug messenger
    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    // instance
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;

    // glfw
    glfwTerminate();
}

Window GraphicsDevice::createWindow(const uint32_t width, const uint32_t height, const char* name) {
    // create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* vkWindow = glfwCreateWindow(width, height, name, nullptr, nullptr);

    // create surface
    VkSurfaceKHR vkSurface;
    assert(glfwCreateWindowSurface(m_instance, vkWindow, nullptr, &vkSurface) == VK_SUCCESS);

    // create swap chain
    VkSwapchainKHR vkSwapChain;
    VkExtent2D vkSwapChainExtent;
    VkFormat vkSwapChainImageFormat;
    std::vector<VkImage> vkSwapChainImages;
    std::vector<VkImageView> vkSwapChainImageViews;
    VkQueue vkPresentQueue;
    {
        // query swap chain support
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
        {
            // capabilities
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, vkSurface, &capabilities);

            // formats
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, vkSurface, &formatCount, nullptr);
            if (formatCount != 0) {
                formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, vkSurface, &formatCount, formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, vkSurface, &presentModeCount, nullptr);
            if (presentModeCount != 0) {
                presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, vkSurface, &presentModeCount,
                                                          presentModes.data());
            }
        }

        // choose surface format
        VkSurfaceFormatKHR surfaceFormat;
        {
            surfaceFormat = formats[0];

            for (const auto& availableFormat : formats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                    availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    surfaceFormat = availableFormat;
                }
            }

            vkSwapChainImageFormat = surfaceFormat.format;
        }

        // choose present mode
        VkPresentModeKHR presentMode;
        {
            presentMode = VK_PRESENT_MODE_FIFO_KHR;

            for (const auto& availablePresentMode : presentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    presentMode = availablePresentMode;
                }
            }
        }

        // choose extent
        {
            vkSwapChainExtent = capabilities.currentExtent;

            if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
                int width, height;
                glfwGetFramebufferSize(vkWindow, &width, &height);

                VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

                actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                                 capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                                 capabilities.maxImageExtent.height);

                vkSwapChainExtent = actualExtent;
            }
        }

        // image count
        uint32_t imageCount = capabilities.minImageCount + 1;
        {
            if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
                imageCount = capabilities.maxImageCount;
            }
        }

        // get present queue and create swap chain
        {
            uint32_t presentFamily  = VK_QUEUE_FAMILY_IGNORED;
            bool presentFamilyFound = false;
            for (int i = 0; i < m_queueFamilies.size(); i++) {
                const auto& queueFamily = m_queueFamilies[i];

                // present family
                // TODO: compare same family with graphics queue
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, vkSurface, &presentSupport);
                if (presentSupport) {
                    presentFamily      = i;
                    presentFamilyFound = true;
                    break;
                }
            }

            assert(presentFamilyFound);

            // present queue
            vkGetDeviceQueue(m_device, presentFamily, 0, &vkPresentQueue);

            // swap chain
            assert(ivkCreateSwapChain(m_device, vkSurface, surfaceFormat, presentMode, capabilities.currentTransform,
                                      imageCount, {m_graphicsFamily, presentFamily}, vkSwapChainExtent,
                                      &vkSwapChain) == VK_SUCCESS);
        }

        // images
        vkGetSwapchainImagesKHR(m_device, vkSwapChain, &imageCount, nullptr);
        vkSwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, vkSwapChain, &imageCount, vkSwapChainImages.data());

        // image views
        vkSwapChainImageViews.resize(vkSwapChainImages.size());

        for (size_t i = 0; i < vkSwapChainImageViews.size(); i++) {
            assert(ivkCreateImageView(m_device, vkSwapChainImages[i], vkSwapChainImageFormat,
                                      &vkSwapChainImageViews[i]) == VK_SUCCESS);
        }
    }

    Window window                  = OZ_CREATE_VK_DATA(Window);
    window->vkWindow               = vkWindow;
    window->vkSurface              = vkSurface;
    window->vkSwapChain            = vkSwapChain;
    window->vkSwapChainExtent      = std::move(vkSwapChainExtent);
    window->vkSwapChainImageFormat = vkSwapChainImageFormat;
    window->vkSwapChainImages      = std::move(vkSwapChainImages);
    window->vkSwapChainImageViews  = std::move(vkSwapChainImageViews);
    window->vkPresentQueue         = vkPresentQueue;
    window->vkInstance             = m_instance;

    return window;
}

CommandBuffer GraphicsDevice::createCommandBuffer() { return {m_device, m_commandPool}; }

Shader GraphicsDevice::createShader(const std::string& path, ShaderStage stage) {
    std::string absolutePath = file::getBuildPath() + "/oz/resources/shaders/";
    absolutePath += path;
    auto code = file::readFile(absolutePath);

    VkShaderModule shaderModule;
    assert(ivkCreateShaderModule(m_device, code.size(), reinterpret_cast<const uint32_t*>(code.data()),
                                 &shaderModule) == VK_SUCCESS);

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage  = (VkShaderStageFlagBits)stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName  = "main";

    Shader shaderData                           = OZ_CREATE_VK_DATA(Shader);
    shaderData->stage                           = stage;
    shaderData->vkShaderModule                  = shaderModule;
    shaderData->vkPipelineShaderStageCreateInfo = shaderStageInfo;

    return shaderData;
}

RenderPass GraphicsDevice::createRenderPass(Shader vertexShader, Shader fragmentShader, Window window) {
    VkRenderPass vkRenderPass;
    assert(ivkCreateRenderPass(m_device, window->vkSwapChainImageFormat, &vkRenderPass) == VK_SUCCESS);

    VkPipelineLayout vkPipelineLayout;
    assert(ivkCreatePipelineLayout(m_device, &vkPipelineLayout) == VK_SUCCESS);

    VkPipeline vkGraphicsPipeline;
    assert(ivkCreateGraphicsPipeline(
               m_device,
               std::array<VkPipelineShaderStageCreateInfo, 2>{vertexShader->vkPipelineShaderStageCreateInfo,
                                                              fragmentShader->vkPipelineShaderStageCreateInfo}
                   .data(),
               2, window->vkSwapChainExtent, vkPipelineLayout, vkRenderPass, &vkGraphicsPipeline) == VK_SUCCESS);

    std::vector<VkFramebuffer> vkFrameBuffers(window->vkSwapChainImageViews.size());
    for (size_t i = 0; i < window->vkSwapChainImageViews.size(); i++) {
        assert(ivkCreateFramebuffer(m_device, vkRenderPass, window->vkSwapChainExtent, window->vkSwapChainImageViews[i],
                                    &vkFrameBuffers[i]) == VK_SUCCESS);
    }

    RenderPass renderPass          = OZ_CREATE_VK_DATA(RenderPass);
    renderPass->vkRenderPass       = vkRenderPass;
    renderPass->vkPipelineLayout   = vkPipelineLayout;
    renderPass->vkGraphicsPipeline = vkGraphicsPipeline;
    renderPass->vkExtent           = window->vkSwapChainExtent;
    renderPass->vkFrameBuffers     = std::move(vkFrameBuffers);

    return renderPass;
}

Semaphore GraphicsDevice::createSemaphore() {
    VkSemaphore vkSemaphore;
    assert(ivkCreateSemaphore(m_device, &vkSemaphore) == VK_SUCCESS);

    Semaphore semaphore    = OZ_CREATE_VK_DATA(Semaphore);
    semaphore->vkSemaphore = vkSemaphore;

    return semaphore;
}

void GraphicsDevice::waitIdle() const { vkDeviceWaitIdle(m_device); }

Fence GraphicsDevice::createFence() {
    VkFence vkFence;
    assert(ivkCreateFence(m_device, &vkFence) == VK_SUCCESS);

    Fence fence    = OZ_CREATE_VK_DATA(Fence);
    fence->vkFence = vkFence;

    return fence;
}

void GraphicsDevice::waitFences(Fence fence, uint32_t fenceCount, bool waitAll) const {
    vkWaitForFences(m_device, fenceCount, &fence->vkFence, waitAll ? VK_TRUE : VK_FALSE, UINT64_MAX);
}

void GraphicsDevice::resetFences(Fence fence, uint32_t fenceCount) const {
    vkResetFences(m_device, fenceCount, &fence->vkFence);
}

CommandBuffer GraphicsDevice::getNextCommandBuffer() const { return m_commandBuffers[m_currentFrame]; }

void GraphicsDevice::submit(CommandBuffer& commandBuffer) const {
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    assert(ivkQueueSubmit(m_graphicsQueue, m_imageAvailableSemaphores[m_currentFrame]->vkSemaphore, waitStage,
                          commandBuffer.m_vkCommandBuffer, m_renderFinishedSemaphores[m_currentFrame]->vkSemaphore,
                          m_inFlightFences[m_currentFrame]->vkFence) == VK_SUCCESS);
}

bool GraphicsDevice::isWindowOpen(Window window) const { return !glfwWindowShouldClose(window->vkWindow); }

uint32_t GraphicsDevice::getNextImage(Window window) const {
    glfwPollEvents();

    waitFences(m_inFlightFences[m_currentFrame], 1);
    resetFences(m_inFlightFences[m_currentFrame], 1);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_device, window->vkSwapChain, UINT64_MAX,
                          m_imageAvailableSemaphores[m_currentFrame]->vkSemaphore, VK_NULL_HANDLE, &imageIndex);

    return imageIndex;
}

void GraphicsDevice::presentImage(Window window, uint32_t imageIndex) {
    VkResult result = ivkQueuePresent(window->vkPresentQueue, m_renderFinishedSemaphores[m_currentFrame]->vkSemaphore,
                                      window->vkSwapChain, imageIndex);
    assert(result == VK_SUCCESS);

    m_currentFrame = (m_currentFrame + 1) % FRAMES_IN_FLIGHT;
}

void GraphicsDevice::free(Window window) const { OZ_FREE_VK_DATA(window); }
void GraphicsDevice::free(Shader shader) const { OZ_FREE_VK_DATA(shader); }
void GraphicsDevice::free(RenderPass renderPass) const { OZ_FREE_VK_DATA(renderPass) }
void GraphicsDevice::free(Semaphore semaphore) const { OZ_FREE_VK_DATA(semaphore); }
void GraphicsDevice::free(Fence fence) const { OZ_FREE_VK_DATA(fence); }

VkCommandPool GraphicsDevice::_createCommandPool() {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    assert(ivkCreateCommandPool(m_device, m_graphicsFamily, &commandPool) == VK_SUCCESS);

    return commandPool;
}

} // namespace oz::gfx::vk