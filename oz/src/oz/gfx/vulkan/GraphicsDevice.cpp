#include "oz/gfx/vulkan/GraphicsDevice.h"
#include "oz/core/file/file.h"
#include "oz/gfx/vulkan/vk_data.h"
#include "oz/gfx/vulkan/vk_utils.h"

namespace oz::gfx::vk {

namespace {
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
}

GraphicsDevice::~GraphicsDevice() {
    // command pool
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;

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

    Window window                  = new WindowData;
    window->vkWindow               = vkWindow;
    window->vkSurface              = vkSurface;
    window->vkSwapChain            = vkSwapChain;
    window->vkSwapChainExtent      = std::move(vkSwapChainExtent);
    window->vkSwapChainImageFormat = vkSwapChainImageFormat;
    window->vkSwapChainImages      = std::move(vkSwapChainImages);
    window->vkSwapChainImageViews  = std::move(vkSwapChainImageViews);
    window->vkPresentQueue         = vkPresentQueue;

    return window;
}

CommandBuffer GraphicsDevice::createCommandBuffer() { return {m_device, m_commandPool}; }

Shader GraphicsDevice::createShader(const std::string& path, ShaderStage stage) {
    std::string absolutePath = file::getBuildPath() + "/oz/resources/shaders/";
    absolutePath += path;
    auto code = file::readFile(absolutePath);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    assert(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS);

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage  = (VkShaderStageFlagBits)stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName  = "main";

    Shader shaderData                           = new ShaderData;
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
        assert(gfx::vk::ivkCreateFramebuffer(m_device, vkRenderPass, window->vkSwapChainExtent,
                                             window->vkSwapChainImageViews[i], &vkFrameBuffers[i]) == VK_SUCCESS);
    }

    RenderPass renderPass          = new RenderPassData;
    renderPass->vkRenderPass       = vkRenderPass;
    renderPass->vkPipelineLayout   = vkPipelineLayout;
    renderPass->vkGraphicsPipeline = vkGraphicsPipeline;
    renderPass->vkFrameBuffers     = vkFrameBuffers;
    renderPass->vkExtent           = window->vkSwapChainExtent;

    return renderPass;
}

Semaphore GraphicsDevice::createSemaphore() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore vkSemaphore;
    vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &vkSemaphore);

    Semaphore semaphore    = new SemaphoreData;
    semaphore->vkSemaphore = vkSemaphore;

    return semaphore;
}

Fence GraphicsDevice::createFence() {
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkFence vkFence;
    vkCreateFence(m_device, &fenceInfo, nullptr, &vkFence);

    Fence fence    = new FenceData;
    fence->vkFence = vkFence;

    return fence;
}

void GraphicsDevice::waitFences(Fence fence, uint32_t fenceCount, bool waitAll) {
    vkWaitForFences(m_device, fenceCount, &fence->vkFence, waitAll ? VK_TRUE : VK_FALSE, UINT64_MAX);
}

void GraphicsDevice::resetFences(Fence fence, uint32_t fenceCount) {
    vkResetFences(m_device, fenceCount, &fence->vkFence);
}

void GraphicsDevice::free(Window window) {
    // swap chain
    vkDestroySwapchainKHR(m_device, window->vkSwapChain, nullptr);
    window->vkSwapChain = VK_NULL_HANDLE;

    // image views
    for (auto imageView : window->vkSwapChainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    window->vkSwapChainImageViews.clear();

    // images
    // for (auto image : window->vkSwapChainImages) {
    //    vkDestroyImage(m_device, image, nullptr);
    // }
    window->vkSwapChainImages.clear();

    // surface
    if (window->vkSurface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, window->vkSurface, nullptr);
        window->vkSurface = VK_NULL_HANDLE;
    }

    // window
    if (window->vkWindow != nullptr) {
        glfwDestroyWindow(window->vkWindow);
        window->vkWindow = nullptr;
    }

    delete window;
}

void GraphicsDevice::free(Shader shader) {
    vkDestroyShaderModule(m_device, shader->vkShaderModule, nullptr);
    delete shader;
}

void GraphicsDevice::free(RenderPass renderPass) {
    vkDestroyPipeline(m_device, renderPass->vkGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_device, renderPass->vkPipelineLayout, nullptr);
    vkDestroyRenderPass(m_device, renderPass->vkRenderPass, nullptr);

    for (auto framebuffer : renderPass->vkFrameBuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    renderPass->vkFrameBuffers.clear();

    delete renderPass;
}

void GraphicsDevice::free(Semaphore semaphore) {
    vkDestroySemaphore(m_device, semaphore->vkSemaphore, nullptr);
    delete semaphore;
}
void GraphicsDevice::free(Fence fence) {
    vkDestroyFence(m_device, fence->vkFence, nullptr);
    delete fence;
}

VkCommandPool GraphicsDevice::_createCommandPool() {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkResult result           = ivkCreateCommandPool(m_device, m_graphicsFamily, &commandPool);

    assert(result == VK_SUCCESS);

    return commandPool;
}

} // namespace oz::gfx::vk
