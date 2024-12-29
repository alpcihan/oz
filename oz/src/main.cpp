#include "oz/gfx/vulkan/GraphicsDevice.h"
using namespace oz::gfx::vk;

int main() {
    GraphicsDevice device(true);
    Window window = device.createWindow(800, 600, "oz");

    Shader vertShader     = device.createShader("default_vert.spv", ShaderStage::Vertex);
    Shader fragShader     = device.createShader("default_frag.spv", ShaderStage::Fragment);
    RenderPass renderPass = device.createRenderPass(vertShader, fragShader, window);

    while (device.isWindowOpen(window)) {
        uint32_t imageIndex = device.getNextImage(window);

        auto cmd = device.getNextCommandBuffer();
        cmd.begin();
        cmd.beginRenderPass(renderPass, imageIndex);
        cmd.draw(3);
        cmd.endRenderPass();
        cmd.end();

        device.submit(cmd);
        device.presentImage(window, imageIndex);
    }
    device.waitIdle();

    device.free(vertShader);
    device.free(fragShader);
    device.free(window);
    device.free(renderPass);

    return 0;
}