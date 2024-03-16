#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "bee/base.h"

namespace bee {

struct WindowProps {
    std::string name = "bee";
    uint32_t width = 1280;
    uint32_t height = 720;
};

class Window {
public:
    Window(const WindowProps& props);
    virtual ~Window();

protected:

private:
    GLFWwindow* m_window;
    
    friend class VulkanGraphicsDevice;
};

}