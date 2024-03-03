#pragma once

#include "bee/platform/vulkan/common.h"

namespace bee {
namespace vk {

struct ContextConfig {
    // extensions
    size_t requiredExtensionsCount = 0;
    const char** requiredExtensions = nullptr;
};

class Context final {
public:
    Context();
    ~Context();

    GLFWwindow* createWindow(uint32_t width, uint32_t height, const char* name = ""); // TODO: move to a window class
    VkInstance get() { return m_instance; }
    VkSurfaceKHR getSurface() { return m_surface; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    // window (TODO: move to a window class)
    GLFWwindow* m_window = nullptr;

private:
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    void _setupDebugMessenger();
    void _destroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator = nullptr);
};

}
}