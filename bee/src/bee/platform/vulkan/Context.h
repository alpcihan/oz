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
    VkSurfaceKHR getVkSurface() { return m_surface; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsFamily = VK_QUEUE_FAMILY_IGNORED;

    // window (TODO: move to a window class)
    GLFWwindow* m_window = nullptr; 
};

}
}