#include "oz/oz.h"
using namespace oz::gfx::vk;

int main() {
    GraphicsDevice device(true);
    Window window = device.createWindow(800, 600, "oz");

    Shader vertShader     = device.createShader("default.vert", ShaderStage::Vertex);
    Shader fragShader     = device.createShader("default.frag", ShaderStage::Fragment);
    RenderPass renderPass = device.createRenderPass(vertShader, fragShader, window);

    while (device.isWindowOpen(window)) {
        uint32_t imageIndex = device.getCurrentImage(window);
        CommandBuffer cmd = device.getCurrentCommandBuffer();

        device.beginCmd(cmd);
        device.beginRenderPass(cmd, renderPass, imageIndex);
        device.draw(cmd, 3);
        device.endRenderPass(cmd);
        device.endCmd(cmd);

        device.submitCmd(cmd);
        device.presentImage(window, imageIndex);
    }
    device.waitIdle();

    device.free(vertShader);
    device.free(fragShader);
    device.free(window);
    device.free(renderPass);

    return 0;
}