#pragma once

#include "bee/platform/vulkan/common.h"

namespace bee {
namespace vk {

class Context final {
public:
    Context();
    ~Context();

    GLFWwindow* createWindow(uint32_t width, uint32_t height, const char* name = "");  // TODO: move to a window class
    
    VkInstance getVkInstance() { return m_instance; }
    VkDevice getVkDevice() { return m_device;}
    VkPhysicalDevice getVkPhysicalDevice() { return m_physicalDevice; }
    VkQueue getVkGraphicsQueue() { return m_graphicsQueue; }
    VkQueue getVkPresentQueue() { return m_presentQueue; }
    VkSurfaceKHR getVkSurface() { return m_surface; }
    VkSwapchainKHR getVkSwapchain() { return m_swapChain; }
    VkFormat getVkSwapchainImageFormat() { return m_swapChainImageFormat; }
    VkExtent2D getVkSwapchainExtent() { return m_swapChainExtent; }
    std::vector<VkImage>& getSwapChainImages() { return m_swapChainImages; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImage> m_swapChainImages;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    
    std::vector<VkQueueFamilyProperties> m_queueFamilies;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsFamily = VK_QUEUE_FAMILY_IGNORED;
    uint32_t m_presentFamily =  VK_QUEUE_FAMILY_IGNORED;

    // window (TODO: move to a window class)
    GLFWwindow* m_window = nullptr; 
};

}
}