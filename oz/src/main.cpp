#include "oz/gfx/vulkan/vk_objects_internal.h"
#include "oz/oz.h"
#include <glm/glm.hpp>
using namespace oz::gfx::vk;

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding   = 0;
        bindingDescription.stride    = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset   = offsetof(Vertex, pos);

        attributeDescriptions[1].binding  = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset   = offsetof(Vertex, color);
        return attributeDescriptions;
    }
};

const std::vector<Vertex> vertices       = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
VkDeviceSize              vertBufferSize = sizeof(vertices[0]) * vertices.size();

const std::vector<uint16_t> indices       = {0, 1, 2, 2, 3, 0};
VkDeviceSize                idxBufferSize = sizeof(indices[0]) * indices.size();

int main() {
    GraphicsDevice device(true);
    Window         window = device.createWindow(800, 600, "oz");

    Shader vertShader = device.createShader("default.vert", ShaderStage::Vertex);
    Shader fragShader = device.createShader("default.frag", ShaderStage::Fragment);

    // create vertex buffer //
    Buffer vertStageBuffer = device.createBuffer(BufferType::Staging, vertBufferSize, vertices.data());
    Buffer vertexBuffer = device.createBuffer(BufferType::Vertex, vertBufferSize);
    device.copyBuffer(vertStageBuffer, vertexBuffer, vertBufferSize);
    device.free(vertStageBuffer);

    // create index buffer //
    Buffer idxStageBuffer = device.createBuffer(BufferType::Staging, idxBufferSize, indices.data());
    Buffer indexBuffer = device.createBuffer(BufferType::Index, idxBufferSize);
    device.copyBuffer(idxStageBuffer, indexBuffer, idxBufferSize);
    device.free(idxStageBuffer);

    // pipeline vertex input //
    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    // create render pass //
    RenderPass renderPass = device.createRenderPass(vertShader, fragShader, window, &vertexInputInfo);

    while (device.isWindowOpen(window)) {
        uint32_t      imageIndex = device.getCurrentImage(window);
        CommandBuffer cmd        = device.getCurrentCommandBuffer();

        device.beginCmd(cmd);
        device.beginRenderPass(cmd, renderPass, imageIndex);
        device.bindVertexBuffer(cmd, vertexBuffer);
        device.bindIndexBuffer(cmd, indexBuffer);
        device.drawIndexed(cmd, indices.size());
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

    device.free(vertexBuffer);
    device.free(indexBuffer);

    return 0;
}