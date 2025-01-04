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

const std::vector<Vertex> vertices = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                                      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                                      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};

int main() {
    GraphicsDevice device(true);
    Window         window = device.createWindow(800, 600, "oz");

    Shader vertShader = device.createShader("default.vert", ShaderStage::Vertex);
    Shader fragShader = device.createShader("default.frag", ShaderStage::Fragment);

    Buffer         vertexBuffer;
    VkBuffer       vkVertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    // create vertex buffer
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size        = sizeof(vertices[0]) * vertices.size();
        bufferInfo.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // create buffer //
        if (vkCreateBuffer(device.m_device, &bufferInfo, nullptr, &vkVertexBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        // memory satisfies requirements //
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device.m_device, vkVertexBuffer, &memRequirements);

        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(device.m_physicalDevice, &memProperties);

        uint32_t              typeFilter = memRequirements.memoryTypeBits;
        VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        uint32_t              i          = 0;
        for (; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                break;
            }
        }
        assert(i <= memProperties.memoryTypeCount);

        // allocate memory //
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = i;

        if (vkAllocateMemory(device.m_device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        // bind buffer memory //
        vkBindBufferMemory(device.m_device, vkVertexBuffer, vertexBufferMemory, 0);

        // fill the buffer //
        void* data;
        vkMapMemory(device.m_device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferInfo.size);
        vkUnmapMemory(device.m_device, vertexBufferMemory);

        vertexBuffer = device.createBuffer(vkVertexBuffer);
    }

    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    RenderPass renderPass = device.createRenderPass(vertShader, fragShader, window, &vertexInputInfo);

    while (device.isWindowOpen(window)) {
        uint32_t      imageIndex = device.getCurrentImage(window);
        CommandBuffer cmd        = device.getCurrentCommandBuffer();

        device.beginCmd(cmd);
        device.beginRenderPass(cmd, renderPass, imageIndex);
        device.bindVertexBuffer(cmd, vertexBuffer);
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

    vkDestroyBuffer(device.m_device, vkVertexBuffer, nullptr);
    vkFreeMemory(device.m_device, vertexBufferMemory, nullptr);

    return 0;
}