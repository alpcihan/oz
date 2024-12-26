#include "oz/gfx/vulkan/GraphicsDevice.h"
#include "oz/core/file/file.h"
#include "oz/gfx/vulkan/vk_data.h"
#include "oz/gfx/vulkan/vk_utils.h"

namespace oz::gfx::vk {

namespace {
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {
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
    const char **extensions = glfwGetRequiredInstanceExtensions(&extensionCount);

    const std::vector<const char *> requiredExtensions         = ivkPopulateExtensions();
    const std::vector<const char *> requiredInstanceExtensions = ivkPopulateInstanceExtensions(extensions, extensionCount, enableValidationLayers);
    const std::vector<const char *> layers                     = ivkPopulateLayers(enableValidationLayers);

    assert(ivkAreLayersSupported(layers));

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo =
        enableValidationLayers ? ivkPopulateDebugMessengerCreateInfo(debugCallback) : VkDebugUtilsMessengerCreateInfoEXT{};

    VkResult result;

    // create instance
    result = ivkCreateInstance(
        static_cast<uint32_t>(requiredInstanceExtensions.size()), requiredInstanceExtensions.data(), static_cast<uint32_t>(layers.size()), layers.data(),
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

    // image views
    for (auto imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    m_swapChainImageViews.clear();

    // swap chain
    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    m_swapChain = VK_NULL_HANDLE;

    // device
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;

    // debug utils
    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    // surface
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }

    // instance
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;

    // window (TODO: move to a window class)
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
    }
}

GLFWwindow *GraphicsDevice::createWindow(const uint32_t width, const uint32_t height, const char *name) {
    // create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, name, nullptr, nullptr);

    // create surface
    assert(glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) == VK_SUCCESS);

    // create swap chain
    {
        // query swap chain support
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
        {
            // capabilities
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);

            // formats
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
            if (formatCount != 0) {
                formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, nullptr);
            if (presentModeCount != 0) {
                presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, &presentModeCount, presentModes.data());
            }
        }

        // choose surface format
        VkSurfaceFormatKHR surfaceFormat;
        {
            surfaceFormat = formats[0];

            for (const auto &availableFormat : formats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    surfaceFormat = availableFormat;
                }
            }

            m_swapChainImageFormat = surfaceFormat.format;
        }

        // choose present mode
        VkPresentModeKHR presentMode;
        {
            presentMode = VK_PRESENT_MODE_FIFO_KHR;

            for (const auto &availablePresentMode : presentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    presentMode = availablePresentMode;
                }
            }
        }

        // choose extent
        VkExtent2D extent;
        {
            extent = capabilities.currentExtent;

            if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
                int width, height;
                glfwGetFramebufferSize(m_window, &width, &height);

                VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

                actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                extent = actualExtent;
            }

            m_swapChainExtent = extent;
        }

        // image count
        uint32_t imageCount = capabilities.minImageCount + 1;
        {
            if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
                imageCount = capabilities.maxImageCount;
            }
        }

        // get present queue
        {
            bool presentFamilyFound = false;
            for (int i = 0; i < m_queueFamilies.size(); i++) {
                const auto &queueFamily = m_queueFamilies[i];

                // present family
                // TODO: compare same family with graphics queue
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &presentSupport);
                if (presentSupport) {
                    m_presentFamily    = i;
                    presentFamilyFound = true;
                    break;
                }
            }

            assert(presentFamilyFound);

            vkGetDeviceQueue(m_device, m_presentFamily, 0, &m_presentQueue);
        }

        // swap chain
        assert(
            ivkCreateSwapChain(
                m_device, m_surface, surfaceFormat, presentMode, capabilities.currentTransform, imageCount, {m_graphicsFamily, m_presentFamily}, extent,
                &m_swapChain) == VK_SUCCESS);

        // images
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

        // image views
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++) {
            assert(ivkCreateImageView(m_device, m_swapChainImages[i], m_swapChainImageFormat, &m_swapChainImageViews[i]) == VK_SUCCESS);
        }
    }

    return m_window;
}

VkCommandBuffer GraphicsDevice::createCommandBuffer() {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkResult result = ivkAllocateCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);

    assert(result == VK_SUCCESS);

    return commandBuffer;
}

Shader GraphicsDevice::createShader(const std::string &path, ShaderStage stage) {
    std::string absolutePath = file::getBuildPath() + "/oz/resources/shaders/";
    absolutePath += path;
    auto code = file::readFile(absolutePath);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage  = (VkShaderStageFlagBits)stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName  = "main";

    Shader shaderData          = new ShaderData;
    shaderData->stage          = stage;
    shaderData->vkShaderModule = shaderModule;
    shaderData->stageInfo      = shaderStageInfo;

    return shaderData;
}

void GraphicsDevice::free(Shader shader) {
    vkDestroyShaderModule(m_device, shader->vkShaderModule, nullptr);
    delete shader;
}

VkCommandPool GraphicsDevice::_createCommandPool() {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkResult result           = ivkCreateCommandPool(m_device, m_graphicsFamily, &commandPool);

    assert(result == VK_SUCCESS);

    return commandPool;
}

} // namespace oz::gfx::vk
