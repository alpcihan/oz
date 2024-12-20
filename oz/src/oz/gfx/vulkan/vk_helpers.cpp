#include "oz/gfx/vulkan/vk_helpers.h"

namespace oz::gfx {

VkResult ivkCreateInstance(
    uint32_t extensionCount,
    const char *const *extensionNames,
    uint32_t layerCount,
    const char *const *layerNames,
    VkDebugUtilsMessengerCreateInfoEXT *debugCreateInfo,
    VkInstance *outInstance) {
    // app info
    const VkApplicationInfo appInfo{
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "oz",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "oz",
        .apiVersion         = VK_API_VERSION_1_0};

    // instance create info
    const VkInstanceCreateInfo createInfo{
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = debugCreateInfo,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = layerCount,
        .ppEnabledLayerNames     = layerNames,
        .enabledExtensionCount   = extensionCount,
        .ppEnabledExtensionNames = extensionNames,
        .flags                   = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR};

    return vkCreateInstance(&createInfo, nullptr, outInstance);
}

VkResult ivkCreateDebugMessenger(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT &debugCreateInfo, VkDebugUtilsMessengerEXT *outDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // TODO: check func != nullptr
    return func(instance, &debugCreateInfo, nullptr, outDebugMessenger);
}

bool ivkPickPhysicalDevice(
    VkInstance instance,
    const std::vector<const char *> &requiredExtensions,
    VkPhysicalDevice *outPhysicalDevice,
    std::vector<VkQueueFamilyProperties> *outQueueFamilies,
    uint32_t *outGraphicsFamily) {
    // get physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    assert(deviceCount > 0);
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    // find "suitable" GPU
    bool isGPUFound = false;
    for (const auto &physicalDevice : physicalDevices) {
        // check queue family support
        std::optional<uint32_t> graphicsFamily;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        outQueueFamilies->resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, outQueueFamilies->data());
        for (int i = 0; i < outQueueFamilies->size(); i++) {
            const auto &queueFamily = (*outQueueFamilies)[i];

            // graphics family
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
                break;
            }
        }
        bool areQueueFamiliesSupported = graphicsFamily.has_value();

        // check extension support
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
        uint32_t extensionMatchCount = 0;
        for (const auto &availableExtension : availableExtensions) {
            for (const auto &requiredExtension : requiredExtensions) {
                if (strcmp(requiredExtension, availableExtension.extensionName) == 0) {
                    extensionMatchCount++;
                }
            }
        }
        bool areExtensionsSupported = extensionMatchCount == (uint32_t)requiredExtensions.size();

        // check if the GPU is suitable
        bool isSuitable = areExtensionsSupported && areQueueFamiliesSupported;

        if (isSuitable) {
            *outGraphicsFamily = graphicsFamily.value();
            *outPhysicalDevice = physicalDevice;
            isGPUFound         = true;
            break;
        }
    }

    return isGPUFound;
}

VkResult ivkCreateLogicalDevice(
    VkPhysicalDevice physicalDevice,
    const std::vector<uint32_t> &uniqueQueueFamilies,
    const std::vector<const char *> &requiredExtensions,
    const std::vector<const char *> &validationLayers,
    VkDevice *outDevice) {

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount       = 1,
            .pQueuePriorities = &queuePriority};

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos       = queueCreateInfos.data(),
        .enabledLayerCount       = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames     = validationLayers.data(),
        .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
        .pEnabledFeatures        = &deviceFeatures};

    return vkCreateDevice(physicalDevice, &createInfo, nullptr, outDevice);
}

VkResult ivkCreateSwapChain(
    VkDevice device,
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR &surfaceFormat,
    VkPresentModeKHR presentMode,
    VkSurfaceTransformFlagBitsKHR preTransform,
    uint32_t imageCount,
    const uint32_t (&queueFamilyIndices)[2],
    const VkExtent2D &extent,
    VkSwapchainKHR *outSwapChain) {

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform     = preTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = presentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = VK_NULL_HANDLE;

    if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;       // optional
        createInfo.pQueueFamilyIndices   = nullptr; // optional
    }

    return vkCreateSwapchainKHR(device, &createInfo, nullptr, outSwapChain);
}

VkResult ivkCreateCommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPool *outCommandPool) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;

    return vkCreateCommandPool(device, &poolInfo, nullptr, outCommandPool);
}

VkResult ivkAllocateCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, VkCommandBuffer *outCommandBuffers) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBufferCount;

    return vkAllocateCommandBuffers(device, &allocInfo, outCommandBuffers);
}

VkDebugUtilsMessengerCreateInfoEXT ivkPopulateDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback) {
    return VkDebugUtilsMessengerCreateInfoEXT{
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = callback};
}

std::vector<const char *> ivkPopulateExtensions() {
    return {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset",
    };
}

std::vector<const char *> ivkPopulateInstanceExtensions(const char **extensions, uint32_t extensionCount, bool isValidationLayerEnabled) {
    std::vector<const char *> requiredInstanceExtensions(extensions, extensions + extensionCount);

    if (isValidationLayerEnabled) {
        requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Add the VK_KHR_portability_enumeration extension
    requiredInstanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    requiredInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    return requiredInstanceExtensions;
}

std::vector<const char *> ivkPopulateLayers(bool isValidationLayerEnabled) {
    typedef std::vector<const char *> ccv;

    return isValidationLayerEnabled ? ccv{"VK_LAYER_KHRONOS_validation"} : ccv{};
}

bool ivkAreLayersSupported(const std::vector<const char *> &layers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : layers) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

} // namespace oz::gfx