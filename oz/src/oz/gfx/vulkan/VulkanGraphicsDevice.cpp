#include "oz/gfx/vulkan/VulkanGraphicsDevice.h"
#include "oz/core/Window.h"
#include "oz/gfx/vulkan/vk_helpers.h"

namespace oz::gfx {

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

VulkanGraphicsDevice::VulkanGraphicsDevice(const bool enableValidationLayers) {
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
        static_cast<uint32_t>(requiredInstanceExtensions.size()), requiredInstanceExtensions.data(), static_cast<uint32_t>(layers.size()),
        layers.data(), enableValidationLayers ? &debugCreateInfo : nullptr, &m_instance);
    assert(result == VK_SUCCESS);

    // create debug messenger
    if (enableValidationLayers) {
        result = ivkCreateDebugMessenger(m_instance, debugCreateInfo, &m_debugMessenger);
        assert(result == VK_SUCCESS);
    }

    // pick physical device
    ivkPickPhysicalDevice(m_instance, requiredExtensions, &m_physicalDevice, &m_queueFamilies, &m_graphicsFamily);
    assert(m_physicalDevice != VK_NULL_HANDLE);

    // create logical device
    result = ivkCreateLogicalDevice(
        m_physicalDevice, {m_graphicsFamily}, // unique queue families (TODO: support multiple queue families)
        requiredExtensions, layers, &m_device);
    assert(result == VK_SUCCESS);

    // get device queues
    vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
}

VulkanGraphicsDevice::~VulkanGraphicsDevice() {
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

GLFWwindow *VulkanGraphicsDevice::createWindow(const uint32_t width, const uint32_t height, const char *name) {
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

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface          = m_surface;
        createInfo.minImageCount    = imageCount;
        createInfo.imageFormat      = surfaceFormat.format;
        createInfo.imageColorSpace  = surfaceFormat.colorSpace;
        createInfo.imageExtent      = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.preTransform     = capabilities.currentTransform;
        createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode      = presentMode;
        createInfo.clipped          = VK_TRUE;
        createInfo.oldSwapchain     = VK_NULL_HANDLE;

        uint32_t queueFamilyIndices[2] = {m_graphicsFamily, m_presentFamily};
        if (m_graphicsFamily != m_presentFamily) {
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;       // optional
            createInfo.pQueueFamilyIndices   = nullptr; // optional
        }

        assert(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) == VK_SUCCESS);

        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent      = extent;
    }

    return m_window;
}

VkCommandPool VulkanGraphicsDevice::createCommandPool() {
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkResult result           = ivkCreateCommandPool(m_device, m_graphicsFamily, &commandPool);

    assert(result == VK_SUCCESS);

    return commandPool;
}

VkCommandBuffer VulkanGraphicsDevice::createCommandBuffer(VkCommandPool commandPool) {
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

    VkResult result = ivkAllocateCommandBuffers(m_device, commandPool, 1, &commandBuffer);

    assert(result == VK_SUCCESS);

    return commandBuffer;
}

} // namespace oz::gfx
