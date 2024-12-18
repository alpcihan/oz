#include <iostream>
#include "oz/gfx/vulkan/Dev.h"

int main() {
    oz::vk::DevApp app;
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}