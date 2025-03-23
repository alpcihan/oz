#include "oz/oz.h"
using namespace oz::gfx::vk;

// vertex data
struct Vertex {
    glm::vec2 pos;
    glm::vec3 col;
};

const std::vector<Vertex> vertices       = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
VkDeviceSize              vertBufferSize = sizeof(vertices[0]) * vertices.size();

// index data
const std::vector<uint16_t> indices       = {0, 1, 2, 2, 3, 0};
VkDeviceSize                idxBufferSize = sizeof(indices[0]) * indices.size();

int main() {
    GraphicsDevice device(true);
    Window         window = device.createWindow(800, 600, "oz");

    // create shaders
    Shader vertShader = device.createShader("default.vert", ShaderStage::Vertex);
    Shader fragShader = device.createShader("default.frag", ShaderStage::Fragment);

    // create vertex buffer
    Buffer vertStageBuffer = device.createBuffer(BufferType::Staging, vertBufferSize, vertices.data());
    Buffer vertexBuffer    = device.createBuffer(BufferType::Vertex, vertBufferSize);
    device.copyBuffer(vertStageBuffer, vertexBuffer, vertBufferSize);
    device.free(vertStageBuffer);

    // create index buffer
    Buffer idxStageBuffer = device.createBuffer(BufferType::Staging, idxBufferSize, indices.data());
    Buffer indexBuffer    = device.createBuffer(BufferType::Index, idxBufferSize);
    device.copyBuffer(idxStageBuffer, indexBuffer, idxBufferSize);
    device.free(idxStageBuffer);

    // create render pass
    RenderPass renderPass = device.createRenderPass(vertShader,
                                                    fragShader,
                                                    window,
                                                    VertexLayout(sizeof(Vertex),
                                                                 {VertexLayoutAttribute(offsetof(Vertex, pos), Format::R32G32_SFLOAT),
                                                                  VertexLayoutAttribute(offsetof(Vertex, col), Format::R32G32B32_SFLOAT)}));

    // render loop
    while (device.isWindowOpen(window)) {
        uint32_t      imageIndex = device.getCurrentImage(window);
        CommandBuffer cmd        = device.getCurrentCommandBuffer();

        device.cmdBegin(cmd);
        device.cmdBeginRenderPass(cmd, renderPass, imageIndex);
        device.cmdBindVertexBuffer(cmd, vertexBuffer);
        device.cmdBindIndexBuffer(cmd, indexBuffer);
        device.cmdDrawIndexed(cmd, indices.size());
        device.cmdEndRenderPass(cmd);
        device.cmdEnd(cmd);

        device.cmdSubmit(cmd);
        device.presentImage(window, imageIndex);
    }
    device.waitIdle();

    // free resources
    device.free(vertShader);
    device.free(fragShader);
    device.free(window);
    device.free(renderPass);
    device.free(vertexBuffer);
    device.free(indexBuffer);

    return 0;
}