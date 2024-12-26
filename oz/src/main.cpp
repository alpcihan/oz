#include "oz/gfx/vulkan/Dev.h"
#include <iostream>

int main() {
    oz::DevApp app;
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}