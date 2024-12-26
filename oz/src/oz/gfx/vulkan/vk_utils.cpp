#include "oz/gfx/vulkan/vk_utils.h"

namespace oz::gfx::vk {

VkResult ivkCreateInstance(uint32_t extensionCount,
                           const char* const* extensionNames,
                           uint32_t layerCount,
                           const char* const* layerNames,
                           VkDebugUtilsMessengerCreateInfoEXT* debugCreateInfo,
                           VkInstance* outInstance) {
    // app info
    const VkApplicationInfo appInfo{.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                    .pApplicationName   = "oz",
                                    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                    .pEngineName        = "oz",
                                    .apiVersion         = VK_API_VERSION_1_0};

    // instance create info
    const VkInstanceCreateInfo createInfo{.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                          .pNext                   = debugCreateInfo,
                                          .pApplicationInfo        = &appInfo,
                                          .enabledLayerCount       = layerCount,
                                          .ppEnabledLayerNames     = layerNames,
                                          .enabledExtensionCount   = extensionCount,
                                          .ppEnabledExtensionNames = extensionNames,
                                          .flags                   = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR};

    return vkCreateInstance(&createInfo, nullptr, outInstance);
}

VkResult ivkCreateDebugMessenger(VkInstance instance,
                                 const VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo,
                                 VkDebugUtilsMessengerEXT* outDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    // TODO: check func != nullptr
    return func(instance, &debugCreateInfo, nullptr, outDebugMessenger);
}

bool ivkPickPhysicalDevice(VkInstance instance,
                           const std::vector<const char*>& requiredExtensions,
                           VkPhysicalDevice* outPhysicalDevice,
                           std::vector<VkQueueFamilyProperties>* outQueueFamilies,
                           uint32_t* outGraphicsFamily) {
    // get physical devices
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    assert(deviceCount > 0);
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

    // find "suitable" GPU
    bool isGPUFound = false;
    for (const auto& physicalDevice : physicalDevices) {
        // check queue family support
        std::optional<uint32_t> graphicsFamily;
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
        outQueueFamilies->resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, outQueueFamilies->data());
        for (int i = 0; i < outQueueFamilies->size(); i++) {
            const auto& queueFamily = (*outQueueFamilies)[i];

            // graphics family
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsFamily = i;
                break;
            }
        }
        bool areQueueFamiliesSupported = graphicsFamily.has_value();

        // check extension support
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());
        uint32_t extensionMatchCount = 0;
        for (const auto& availableExtension : availableExtensions) {
            for (const auto& requiredExtension : requiredExtensions) {
                if (strcmp(requiredExtension, availableExtension.extensionName) == 0) {
                    extensionMatchCount++;
                }
            }
        }
        bool areExtensionsSupported = extensionMatchCount == (uint32_t)requiredExtensions.size();

        // check if the GPU is suitable
        bool isSuitable = areExtensionsSupported && areQueueFamiliesSupported;

        if (isSuitable) {
            *outGraphicsFamily = graphicsFamily.value();
            *outPhysicalDevice = physicalDevice;
            isGPUFound         = true;
            break;
        }
    }

    return isGPUFound;
}

VkResult ivkCreateLogicalDevice(VkPhysicalDevice physicalDevice,
                                const std::vector<uint32_t>& uniqueQueueFamilies,
                                const std::vector<const char*>& requiredExtensions,
                                const std::vector<const char*>& validationLayers,
                                VkDevice* outDevice) {
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                                .queueFamilyIndex = queueFamily,
                                                .queueCount       = 1,
                                                .pQueuePriorities = &queuePriority};

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                  .queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size()),
                                  .pQueueCreateInfos       = queueCreateInfos.data(),
                                  .enabledLayerCount       = static_cast<uint32_t>(validationLayers.size()),
                                  .ppEnabledLayerNames     = validationLayers.data(),
                                  .enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size()),
                                  .ppEnabledExtensionNames = requiredExtensions.data(),
                                  .pEnabledFeatures        = &deviceFeatures};

    return vkCreateDevice(physicalDevice, &createInfo, nullptr, outDevice);
}

VkResult ivkCreateSwapChain(VkDevice device,
                            VkSurfaceKHR surface,
                            VkSurfaceFormatKHR& surfaceFormat,
                            VkPresentModeKHR presentMode,
                            VkSurfaceTransformFlagBitsKHR preTransform,
                            uint32_t imageCount,
                            const uint32_t (&queueFamilyIndices)[2],
                            const VkExtent2D& extent,
                            VkSwapchainKHR* outSwapChain) {
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform     = preTransform;
    createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode      = presentMode;
    createInfo.clipped          = VK_TRUE;
    createInfo.oldSwapchain     = VK_NULL_HANDLE;

    if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;       // optional
        createInfo.pQueueFamilyIndices   = nullptr; // optional
    }

    return vkCreateSwapchainKHR(device, &createInfo, nullptr, outSwapChain);
}

VkResult ivkCreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageView* outSwapChainImageView) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image                           = image;
    createInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format                          = format;
    createInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel   = 0;
    createInfo.subresourceRange.levelCount     = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount     = 1;

    return vkCreateImageView(device, &createInfo, nullptr, outSwapChainImageView);
}

VkResult ivkCreateFramebuffer(VkDevice device,
                              VkRenderPass renderPass,
                              VkExtent2D swapchainExtent,
                              VkImageView imageView,
                              VkFramebuffer* outFramebuffer) {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass      = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments    = &imageView;
    framebufferInfo.width           = swapchainExtent.width;
    framebufferInfo.height          = swapchainExtent.height;
    framebufferInfo.layers          = 1;

    return vkCreateFramebuffer(device, &framebufferInfo, nullptr, outFramebuffer);
}

VkResult ivkCreatePipelineLayout(VkDevice device, VkPipelineLayout* outPipelineLayout) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 0;       // No descriptor sets
    pipelineLayoutInfo.pSetLayouts            = nullptr; // No descriptor set layouts
    pipelineLayoutInfo.pushConstantRangeCount = 0;       // No push constant ranges
    pipelineLayoutInfo.pPushConstantRanges    = nullptr; // No push constants

    return vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, outPipelineLayout);
}

VkResult ivkCreateRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkRenderPass* outRenderPass) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format         = swapChainImageFormat;
    colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependency;

    // Create the render pass
    return vkCreateRenderPass(device, &renderPassInfo, nullptr, outRenderPass);
}

VkResult ivkCreateGraphicsPipeline(VkDevice device,
                                   VkPipelineShaderStageCreateInfo* shaderStages,
                                   uint32_t stageCount,
                                   VkExtent2D swapChainExtent,
                                   VkPipelineLayout pipelineLayout,
                                   VkRenderPass renderPass,
                                   VkPipeline* outGraphicsPipeline) {
    std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates    = dynamicStates.data();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 0;
    vertexInputInfo.pVertexBindingDescriptions      = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions    = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swapChainExtent.width);
    viewport.height   = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount          = stageCount;
    pipelineInfo.pStages             = shaderStages;
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = &dynamicState;
    pipelineInfo.layout              = pipelineLayout;
    pipelineInfo.renderPass          = renderPass;
    pipelineInfo.subpass             = 0;

    return vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, outGraphicsPipeline);
}

VkResult ivkCreateCommandPool(VkDevice device, uint32_t queueFamilyIndex, VkCommandPool* outCommandPool) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex;

    return vkCreateCommandPool(device, &poolInfo, nullptr, outCommandPool);
}

VkResult ivkAllocateCommandBuffers(VkDevice device,
                                   VkCommandPool commandPool,
                                   uint32_t commandBufferCount,
                                   VkCommandBuffer* outCommandBuffers) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = commandBufferCount;

    return vkAllocateCommandBuffers(device, &allocInfo, outCommandBuffers);
}

VkDebugUtilsMessengerCreateInfoEXT ivkPopulateDebugMessengerCreateInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback) {
    return VkDebugUtilsMessengerCreateInfoEXT{.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                                              .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                                              .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                                              .pfnUserCallback = callback};
}

std::vector<const char*> ivkPopulateExtensions() {
    return {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_KHR_portability_subset",
    };
}

std::vector<const char*> ivkPopulateInstanceExtensions(const char** extensions,
                                                       uint32_t extensionCount,
                                                       bool isValidationLayerEnabled) {
    std::vector<const char*> requiredInstanceExtensions(extensions, extensions + extensionCount);

    if (isValidationLayerEnabled) {
        requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Add the VK_KHR_portability_enumeration extension
    requiredInstanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    requiredInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    return requiredInstanceExtensions;
}

std::vector<const char*> ivkPopulateLayers(bool isValidationLayerEnabled) {
    typedef std::vector<const char*> ccv;

    return isValidationLayerEnabled ? ccv{"VK_LAYER_KHRONOS_validation"} : ccv{};
}

bool ivkAreLayersSupported(const std::vector<const char*>& layers) {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : layers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

} // namespace oz::gfx::vk