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
    Context(const ContextConfig& config);
    ~Context();

    VkInstance get() { return m_instance; }

private:
    VkInstance m_instance = VK_NULL_HANDLE;

private:
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    void _setupDebugMessenger();
    void _destroyDebugUtilsMessengerEXT(const VkAllocationCallbacks* pAllocator = nullptr);
};

}
}