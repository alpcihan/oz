#include "bee/gfx/vulkan/vk_helpers.h"

namespace bee::gfx {

VkResult ivkCreateInstance(uint32_t extensionCount,
                        const char* const* extensionNames,
                        uint32_t layerCount,
                        const char* const* layerNames,
                        VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo,
                        VkInstance* outInstance) {
    // app info
    const VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "bee",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "bee",
        .apiVersion = VK_API_VERSION_1_0};

    // instance create info
    const VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = debugCreateInfo,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = layerCount,
        .ppEnabledLayerNames = layerNames,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensionNames};

    return vkCreateInstance(&createInfo, nullptr, outInstance);
}

VkResult ivkCreateDebugMessenger(VkInstance instance,
                              const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo,
                              VkDebugUtilsMessengerEXT* outDebugMessenger

) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // TODO: check func != nullptr
    return func(instance, &debugCreateInfo, nullptr, outDebugMessenger);
}

bool ivkPickPhysicalDevice(VkInstance instance,
                        const std::vector<const char*>& requiredExtensions,
                        VkPhysicalDevice* outPhysicalDevice,
                        std::vector<VkQueueFamilyProperties>* outQueueFamilies,
                        uint32_t* outGraphicsFamily) {
    // get physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    assert(deviceCount > 0);
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    // find "suitable" GPU
    bool isGPUFound = false;
    for (const auto& physicalDevice : physicalDevices) {
        // check queue family support
        std::optional<uint32_t> graphicsFamily;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        outQueueFamilies->resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, outQueueFamilies->data());
        for (int i = 0; i < outQueueFamilies->size(); i++) {
            const auto& queueFamily = (*outQueueFamilies)[i];

            // graphics family
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
            }
        }
        bool areQueueFamiliesSupported = graphicsFamily.has_value();

        // check extension support
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
        uint32_t extensionMatchCount = 0;
        for (const auto& availableExtension : availableExtensions) {
            for (const auto& requiredExtension : requiredExtensions) {
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
            isGPUFound = true;
            break;
        }
    }

    return isGPUFound;
}

VkResult ivkCreateLogicalDevice(VkPhysicalDevice physicalDevice,
                             const std::vector<uint32_t>& uniqueQueueFamilies,
                             const std::vector<const char*>& requiredExtensions,
                             const std::vector<const char*>& validationLayers,
                             VkDevice* outDevice) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority};

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
        .pEnabledFeatures = &deviceFeatures};

    return vkCreateDevice(physicalDevice, &createInfo, nullptr, outDevice);
}

std::vector<const char*> ivkPopulateExtensions() {
    return {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

std::vector<const char*> ivkPopulateInstanceExtensions(const char** extensions, uint32_t extensionCount, bool isValidationLayerEnabled) {
    std::vector<const char*> requiredInstanceExtensions(extensions, extensions + extensionCount);

    if (isValidationLayerEnabled) {
        requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    
    return requiredInstanceExtensions;
}

std::vector<const char*> ivkPopulateLayers(bool isValidationLayerEnabled) {
    typedef std::vector<const char*> ccv;

    return isValidationLayerEnabled
               ? ccv{"VK_LAYER_KHRONOS_validation"}
               : ccv{};
}

VkDebugUtilsMessengerCreateInfoEXT ivkPopulateDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback) {
    return VkDebugUtilsMessengerCreateInfoEXT{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = callback};
}

bool ivkAreLayersSupported(const std::vector<const char*>& layers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : layers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
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

}