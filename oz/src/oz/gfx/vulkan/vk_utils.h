#pragma once

#include "oz/gfx/vulkan/vk_base.h"

namespace oz::gfx::vk {

VkResult ivkCreateInstance(uint32_t extensionCount,
                           const char* const* extensionNames,
                           uint32_t layerCount,
                           const char* const* layerNames,
                           VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo,
                           VkInstance* outInstance);

VkResult ivkCreateDebugMessenger(VkInstance instance,
                                 const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo,
                                 VkDebugUtilsMessengerEXT* outDebugMessenger);

bool ivkPickPhysicalDevice(VkInstance instance,
                           const std::vector<const char*>& requiredExtensions,
                           VkPhysicalDevice* outPhysicalDevice,
                           std::vector<VkQueueFamilyProperties>* outQueueFamilies,
                           uint32_t* outGraphicsFamily);

VkResult ivkCreateLogicalDevice(VkPhysicalDevice physicalDevice,
                                const std::vector<uint32_t>& uniqueQueueFamilies,
                                const std::vector<const char*>& requiredExtensions,
                                const std::vector<const char*>& validationLayers,
                                VkDevice* outDevice);

VkResult ivkCreateSwapChain(VkDevice device,
                            VkSurfaceKHR surface,
                            VkSurfaceFormatKHR& surfaceFormat,
                            VkPresentModeKHR presentMode,
                            VkSurfaceTransformFlagBitsKHR preTransform,
                            uint32_t imageCount,
                            const uint32_t (&queueFamilyIndices)[2],
                            const VkExtent2D& extent,
                            VkSwapchainKHR* outSwapChain);

VkResult ivkCreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageView* outSwapChainImageView);

VkResult ivkCreateFramebuffer(VkDevice device,
                              VkRenderPass renderPass,
                              VkExtent2D swapchainExtent,
                              VkImageView imageView,
                              VkFramebuffer* outFramebuffer);

VkResult ivkCreatePipelineLayout(VkDevice device, VkPipelineLayout* outPipelineLayout);

VkResult ivkCreateRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkRenderPass* outRenderPass);

VkResult ivkCreateGraphicsPipeline(VkDevice device,
                                   VkPipelineShaderStageCreateInfo* shaderStages,
                                   uint32_t stageCount,
                                   VkExtent2D swapChainExtent,
                                   VkPipelineLayout pipelineLayout,
                                   VkRenderPass renderPass,
                                   VkPipeline* outGraphicsPipeline);

VkResult ivkCreateCommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPool* outCommandPool);

VkResult ivkAllocateCommandBuffers(VkDevice device,
                                   VkCommandPool commandPool,
                                   uint32_t commandBufferCount,
                                   VkCommandBuffer* outCommandBuffers);

VkResult ivkCreateShaderModule(VkDevice device, size_t codeSize, const uint32_t* code, VkShaderModule* outShaderModule);

VkResult ivkCreateFence(VkDevice device, VkFence* outFence);

VkResult ivkCreateSemaphore(VkDevice device, VkSemaphore* outSemaphore);

VkResult ivkQueueSubmit(VkQueue graphicsQueue, 
                        VkSemaphore imageAvailableSemaphore, 
                        VkPipelineStageFlags waitStage, 
                        VkCommandBuffer commandBuffer, 
                        VkSemaphore renderFinishedSemaphore, 
                        VkFence inFlightFence);

VkResult ivkQueuePresent(VkQueue presentQueue,
                         VkSemaphore renderFinishedSemaphore,
                         VkSwapchainKHR swapChain,
                         uint32_t imageIndex);

VkDebugUtilsMessengerCreateInfoEXT ivkPopulateDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback);

std::vector<const char*> ivkPopulateExtensions();

std::vector<const char*> ivkPopulateInstanceExtensions(const char** extensions,
                                                       uint32_t extensionCount,
                                                       bool isValidationLayerEnabled);

std::vector<const char*> ivkPopulateLayers(bool isValidationLayerEnabled);

bool ivkAreLayersSupported(const std::vector<const char*>& layers);

} // namespace oz::gfx::vk