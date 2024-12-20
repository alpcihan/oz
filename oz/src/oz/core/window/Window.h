#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "oz/base.h"

namespace oz {

struct WindowProps {
    std::string name = "oz";
    uint32_t width   = 1280;
    uint32_t height  = 720;
};

class Window {
  public:
    Window(const WindowProps &props);
    virtual ~Window();

  private:
    GLFWwindow *m_window;

    friend class VulkanGraphicsDevice;
};

} // namespace oz