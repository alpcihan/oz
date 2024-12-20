#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx {

VkResult ivkCreateInstance(
    uint32_t extensionCount,
    const char *const *extensionNames,
    uint32_t layerCount,
    const char *const *layerNames,
    VkDebugUtilsMessengerCreateInfoEXT *debugCreateInfo,
    VkInstance *outInstance);

VkResult ivkCreateDebugMessenger(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT &debugCreateInfo, VkDebugUtilsMessengerEXT *outDebugMessenger);

bool ivkPickPhysicalDevice(
    VkInstance instance,
    const std::vector<const char *> &requiredExtensions,
    VkPhysicalDevice *outPhysicalDevice,
    std::vector<VkQueueFamilyProperties> *outQueueFamilies,
    uint32_t *outGraphicsFamily);

VkResult ivkCreateLogicalDevice(
    VkPhysicalDevice physicalDevice,
    const std::vector<uint32_t> &uniqueQueueFamilies,
    const std::vector<const char *> &requiredExtensions,
    const std::vector<const char *> &validationLayers,
    VkDevice *outDevice);

VkResult ivkCreateSwapChain(
    VkDevice device,
    VkSurfaceKHR surface,
    VkSurfaceFormatKHR &surfaceFormat,
    VkPresentModeKHR presentMode,
    VkSurfaceTransformFlagBitsKHR preTransform,
    uint32_t imageCount,
    const uint32_t (&queueFamilyIndices)[2],
    const VkExtent2D &extent,
    VkSwapchainKHR *outSwapChain);

VkResult ivkCreateCommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPool *outCommandPool);

VkResult ivkAllocateCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, VkCommandBuffer *outCommandBuffers);

VkDebugUtilsMessengerCreateInfoEXT ivkPopulateDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback);

std::vector<const char *> ivkPopulateExtensions();

std::vector<const char *> ivkPopulateInstanceExtensions(const char **extensions, uint32_t extensionCount, bool isValidationLayerEnabled);

std::vector<const char *> ivkPopulateLayers(bool isValidationLayerEnabled);

bool ivkAreLayersSupported(const std::vector<const char *> &layers);

} // namespace oz::gfx