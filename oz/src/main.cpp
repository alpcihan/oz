#include "oz/gfx/vulkan/vk_objects_internal.h"
#include "oz/oz.h"
using namespace oz::gfx::vk;

static constexpr int FRAMES_IN_FLIGHT = 2;

// vertex data //
struct Vertex {
    glm::vec2 pos;
    glm::vec3 col;
};

const std::vector<Vertex> vertices       = {{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                            {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
                                            {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
                                            {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};
VkDeviceSize              vertBufferSize = sizeof(vertices[0]) * vertices.size();

// index data //
const std::vector<uint16_t> indices       = {0, 1, 2, 2, 3, 0};
VkDeviceSize                idxBufferSize = sizeof(indices[0]) * indices.size();

// uniform buffer data //
struct Uniform {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

int main() {
    GraphicsDevice device(true);
    Window         window = device.createWindow(800, 600, "oz");

    // create shaders //
    Shader vertShader = device.createShader("uniform.vert", ShaderStage::Vertex);
    Shader fragShader = device.createShader("default.frag", ShaderStage::Fragment);

    // create vertex buffer //
    Buffer vertStageBuffer = device.createBuffer(BufferType::Staging, vertBufferSize, vertices.data());
    Buffer vertexBuffer    = device.createBuffer(BufferType::Vertex, vertBufferSize);
    device.copyBuffer(vertStageBuffer, vertexBuffer, vertBufferSize);
    device.free(vertStageBuffer);

    // create index buffer //
    Buffer idxStageBuffer = device.createBuffer(BufferType::Staging, idxBufferSize, indices.data());
    Buffer indexBuffer    = device.createBuffer(BufferType::Index, idxBufferSize);
    device.copyBuffer(idxStageBuffer, indexBuffer, idxBufferSize);
    device.free(idxStageBuffer);

    // create uniform buffer //
    std::vector<Buffer> uniformBuffers(FRAMES_IN_FLIGHT);
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        uniformBuffers[i] = device.createBuffer(BufferType::Uniform, sizeof(Uniform));
    }

    // create descriptor set layout //
    DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout();

    // create descriptor pool //


    // create descriptor set //
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = device.m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts        = &descriptorSetLayout->vkDescriptorSetLayout;

    std::vector<VkDescriptorSet> descriptorSets;
    descriptorSets.resize(FRAMES_IN_FLIGHT);

    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        if (vkAllocateDescriptorSets(device.m_device, &allocInfo, &descriptorSets[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }
    }

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i]->vkBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(Uniform);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet           = descriptorSets[i];
        descriptorWrite.dstBinding       = 0;
        descriptorWrite.dstArrayElement  = 0;
        descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount  = 1;
        descriptorWrite.pBufferInfo      = &bufferInfo;
        descriptorWrite.pImageInfo       = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr; // Optional

        vkUpdateDescriptorSets(device.m_device, 1, &descriptorWrite, 0, nullptr);
    }

    // create render pass //
    RenderPass renderPass = device.createRenderPass(vertShader,
                                                    fragShader,
                                                    window,
                                                    VertexLayout(sizeof(Vertex),
                                                                 {VertexLayoutAttribute(offsetof(Vertex, pos), Format::R32G32_SFLOAT),
                                                                  VertexLayoutAttribute(offsetof(Vertex, col), Format::R32G32B32_SFLOAT)}),
                                                    1,
                                                    &descriptorSetLayout->vkDescriptorSetLayout);

    device.free(descriptorSetLayout);

    // rendering loop //
    while (device.isWindowOpen(window)) {
        uint32_t      imageIndex = device.getCurrentImage(window);
        CommandBuffer cmd        = device.getCurrentCommandBuffer();

        // update ubo //
        {
            static auto startTime   = std::chrono::high_resolution_clock::now();
            auto        currentTime = std::chrono::high_resolution_clock::now();
            float       time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
            Uniform     ubo{};
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.proj  = glm::perspective(glm::radians(45.0f), 800 / (float)600, 0.1f, 10.0f);
            // ubo.proj[1][1] *= -1;
            device.updateBuffer(uniformBuffers[device.getCurrentFrame()], &ubo, sizeof(ubo));
        }

        device.beginCmd(cmd);
        device.beginRenderPass(cmd, renderPass, imageIndex);
        device.bindVertexBuffer(cmd, vertexBuffer);
        device.bindIndexBuffer(cmd, indexBuffer);
        vkCmdBindDescriptorSets(cmd->vkCommandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                renderPass->vkPipelineLayout,
                                0,
                                1,
                                &descriptorSets[device.getCurrentFrame()],
                                0,
                                nullptr);

        device.drawIndexed(cmd, indices.size());
        device.endRenderPass(cmd);
        device.endCmd(cmd);

        device.submitCmd(cmd);
        device.presentImage(window, imageIndex);
    }
    device.waitIdle();

    // free resources //
    device.free(vertShader);
    device.free(fragShader);
    device.free(window);
    device.free(renderPass);
    device.free(vertexBuffer);
    device.free(indexBuffer);

    for (auto& uniformBuffer : uniformBuffers) {
        device.free(uniformBuffer);
    }
    uniformBuffers.clear();

    return 0;
}