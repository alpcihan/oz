#include "oz/gfx/vulkan/GraphicsDevice.h"
#include "oz/core/file/file.h"
#include "oz/gfx/vulkan/vk_funcs.h"
#include "oz/gfx/vulkan/vk_objects_internal.h"

#define OZ_VK_ASSERT(x) assert((x) == VK_SUCCESS)

namespace oz::gfx::vk {

namespace {
static constexpr int FRAMES_IN_FLIGHT = 2;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void*                                       pUserData) {
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
    }

    return VK_FALSE;
}

} // namespace

GraphicsDevice::GraphicsDevice(const bool enableValidationLayers) {
    // glfw // TODO: seperate glfw logic
    glfwInit();
    uint32_t     extensionCount = 0;
    const char** extensions     = glfwGetRequiredInstanceExtensions(&extensionCount);

    const std::vector<const char*> requiredExtensions         = ivkPopulateExtensions();
    const std::vector<const char*> requiredInstanceExtensions = ivkPopulateInstanceExtensions(extensions, extensionCount, enableValidationLayers);
    const std::vector<const char*> layers                     = ivkPopulateLayers(enableValidationLayers);

    assert(ivkAreLayersSupported(layers));

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo =
        enableValidationLayers ? ivkPopulateDebugMessengerCreateInfo(debugCallback) : VkDebugUtilsMessengerCreateInfoEXT{};

    // create instance
    {
        // app info
        VkApplicationInfo appInfo{};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "oz";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "oz";
        appInfo.apiVersion         = VK_API_VERSION_1_0;

        // instance create info
        VkInstanceCreateInfo createInfo{};
        createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext                   = &debugCreateInfo;
        createInfo.pApplicationInfo        = &appInfo;
        createInfo.enabledLayerCount       = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames     = layers.data();
        createInfo.enabledExtensionCount   = static_cast<uint32_t>(requiredInstanceExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
        createInfo.flags                   = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        OZ_VK_ASSERT(vkCreateInstance(&createInfo, nullptr, &m_instance));
    }

    // create debug messenger
    if (enableValidationLayers) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
        // TODO: check func != nullptr
        OZ_VK_ASSERT(func(m_instance, &debugCreateInfo, nullptr, &m_debugMessenger));
    }

    // pick physical device
    {
        // get physical devices
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
        assert(deviceCount > 0);
        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, physicalDevices.data());

        // find "suitable" GPU
        bool isGPUFound = false;
        for (const auto& physicalDevice : physicalDevices) {
            // check queue family support
            std::optional<uint32_t> graphicsFamily;
            uint32_t                queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
            m_queueFamilies.resize(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, m_queueFamilies.data());
            for (int i = 0; i < m_queueFamilies.size(); i++) {
                const auto& queueFamily = m_queueFamilies[i];

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
                m_graphicsFamily = graphicsFamily.value();
                m_physicalDevice = physicalDevice;
                isGPUFound = true;
                break;
            }
        }
        assert(isGPUFound);
    }

    // create logical device
    {
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : std::vector<uint32_t>{m_graphicsFamily}) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;

            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames = layers.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        OZ_VK_ASSERT(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device));
    }

    // get device queues
    vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);

    // create a command pool
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_graphicsFamily;

        OZ_VK_ASSERT(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool));
    }

    // init current frame
    m_currentFrame = 0;

    // create command buffers
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
    {
        m_commandBuffers.emplace_back(std::move(createCommandBuffer()));
    }

    // create synchronization objects
    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        m_imageAvailableSemaphores.push_back(createSemaphore());
        m_renderFinishedSemaphores.push_back(createSemaphore());
        m_inFlightFences.push_back(createFence());
    }
}

GraphicsDevice::~GraphicsDevice() {
    // destroy command pool
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    m_commandPool = VK_NULL_HANDLE;

    // destroy synchronization objects
    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        free(m_renderFinishedSemaphores[i]);
        free(m_imageAvailableSemaphores[i]);
        free(m_inFlightFences[i]);
    }
    m_renderFinishedSemaphores.clear();
    m_imageAvailableSemaphores.clear();
    m_inFlightFences.clear();

    // destroy device
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;

    // destroy debug messenger
    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }

    // destroy instance
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;

    // destroy glfw
    glfwTerminate();
}

Window GraphicsDevice::createWindow(const uint32_t width, const uint32_t height, const char* name) {
    // create window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* vkWindow = glfwCreateWindow(width, height, name, nullptr, nullptr);

    // create surface
    VkSurfaceKHR vkSurface;
    assert(glfwCreateWindowSurface(m_instance, vkWindow, nullptr, &vkSurface) == VK_SUCCESS);

    // create swap chain
    VkSwapchainKHR           vkSwapChain;
    VkExtent2D               vkSwapChainExtent;
    VkFormat                 vkSwapChainImageFormat;
    std::vector<VkImage>     vkSwapChainImages;
    std::vector<VkImageView> vkSwapChainImageViews;
    VkQueue                  vkPresentQueue;
    {
        // query swap chain support
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
        {
            // get physical device surface capabilities
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, vkSurface, &capabilities);

            // get physical device surface formats
            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, vkSurface, &formatCount, nullptr);
            if (formatCount != 0) {
                formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, vkSurface, &formatCount, formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, vkSurface, &presentModeCount, nullptr);
            if (presentModeCount != 0) {
                presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, vkSurface, &presentModeCount, presentModes.data());
            }
        }

        // choose surface format
        VkSurfaceFormatKHR surfaceFormat;
        {
            surfaceFormat = formats[0];

            for (const auto& availableFormat : formats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    surfaceFormat = availableFormat;
                }
            }

            vkSwapChainImageFormat = surfaceFormat.format;
        }

        // choose present mode
        VkPresentModeKHR presentMode;
        {
            presentMode = VK_PRESENT_MODE_FIFO_KHR;

            for (const auto& availablePresentMode : presentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    presentMode = availablePresentMode;
                }
            }
        }

        // choose extent
        {
            vkSwapChainExtent = capabilities.currentExtent;

            if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
                int width, height;
                glfwGetFramebufferSize(vkWindow, &width, &height);

                VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

                actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                vkSwapChainExtent = actualExtent;
            }
        }

        // get image count
        uint32_t imageCount = capabilities.minImageCount + 1;
        {
            if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
                imageCount = capabilities.maxImageCount;
            }
        }

        // get present queue and create swap chain
        {
            uint32_t presentFamily      = VK_QUEUE_FAMILY_IGNORED;
            bool     presentFamilyFound = false;
            for (int i = 0; i < m_queueFamilies.size(); i++) {
                const auto& queueFamily = m_queueFamilies[i];

                // present family
                // TODO: compare same family with graphics queue
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, vkSurface, &presentSupport);
                if (presentSupport) {
                    presentFamily      = i;
                    presentFamilyFound = true;
                    break;
                }
            }

            assert(presentFamilyFound);

            // get present queue
            vkGetDeviceQueue(m_device, presentFamily, 0, &vkPresentQueue);

            // create swap chain
            {
                VkSwapchainCreateInfoKHR createInfo{};
                createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
                createInfo.surface          = vkSurface;
                createInfo.minImageCount    = imageCount;
                createInfo.imageFormat      = surfaceFormat.format;
                createInfo.imageColorSpace  = surfaceFormat.colorSpace;
                createInfo.imageExtent      = vkSwapChainExtent;
                createInfo.imageArrayLayers = 1;
                createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                createInfo.preTransform     = capabilities.currentTransform;
                createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
                createInfo.presentMode      = presentMode;
                createInfo.clipped          = VK_TRUE;
                createInfo.oldSwapchain     = VK_NULL_HANDLE;

                if (m_graphicsFamily != presentFamily) {
                    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
                    createInfo.queueFamilyIndexCount = 2;
                    const uint32_t queueFamilyIndices[] = {m_graphicsFamily, presentFamily};
                    createInfo.pQueueFamilyIndices   = queueFamilyIndices;
                } else {
                    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
                    createInfo.queueFamilyIndexCount = 0;       // optional
                    createInfo.pQueueFamilyIndices   = nullptr; // optional
                }

                OZ_VK_ASSERT(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &vkSwapChain));
            }
        }

        // get swap chain images
        vkGetSwapchainImagesKHR(m_device, vkSwapChain, &imageCount, nullptr);
        vkSwapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, vkSwapChain, &imageCount, vkSwapChainImages.data());

        // create image views
        vkSwapChainImageViews.resize(vkSwapChainImages.size());
        for (size_t i = 0; i < vkSwapChainImageViews.size(); i++) {
            assert(ivkCreateImageView(m_device, vkSwapChainImages[i], vkSwapChainImageFormat, &vkSwapChainImageViews[i]) == VK_SUCCESS);
        }
    }

    // crate window object
    Window window                  = OZ_CREATE_VK_OBJECT(Window);
    window->vkWindow               = vkWindow;
    window->vkSurface              = vkSurface;
    window->vkSwapChain            = vkSwapChain;
    window->vkSwapChainExtent      = std::move(vkSwapChainExtent);
    window->vkSwapChainImageFormat = vkSwapChainImageFormat;
    window->vkSwapChainImages      = std::move(vkSwapChainImages);
    window->vkSwapChainImageViews  = std::move(vkSwapChainImageViews);
    window->vkPresentQueue         = vkPresentQueue;
    window->vkInstance             = m_instance;

    return window;
}

CommandBuffer GraphicsDevice::createCommandBuffer() {
    VkCommandBuffer vkCommandBuffer;
    // allocate command buffer
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = m_commandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        OZ_VK_ASSERT(vkAllocateCommandBuffers(m_device, &allocInfo, &vkCommandBuffer));
    }

    // create command buffer object
    CommandBuffer commandBuffer    = OZ_CREATE_VK_OBJECT(CommandBuffer);
    commandBuffer->vkCommandBuffer = vkCommandBuffer;
    return commandBuffer;
}

Shader GraphicsDevice::createShader(const std::string& path, ShaderStage stage) {
    std::string absolutePath = file::getBuildPath() + "/oz/resources/shaders/";
    absolutePath += path + ".spv";
    auto code = file::readFile(absolutePath);

    // create shader module
    VkShaderModule shaderModule;
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

        OZ_VK_ASSERT(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule));
    }

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage  = (VkShaderStageFlagBits)stage;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName  = "main";

    // create shader object
    Shader shaderData                           = OZ_CREATE_VK_OBJECT(Shader);
    shaderData->stage                           = stage;
    shaderData->vkShaderModule                  = shaderModule;
    shaderData->vkPipelineShaderStageCreateInfo = shaderStageInfo;

    return shaderData;
}

RenderPass GraphicsDevice::createRenderPass(Shader vertexShader, Shader fragmentShader, Window window, const VertexLayout& vertexLayout) {
    // create render pass //
    VkRenderPass vkRenderPass;
    OZ_VK_ASSERT(ivkCreateRenderPass(m_device, window->vkSwapChainImageFormat, &vkRenderPass));

    // create pipeline layout //
    VkPipelineLayout vkPipelineLayout;
    OZ_VK_ASSERT(ivkCreatePipelineLayout(m_device, &vkPipelineLayout));

    // create vertex state input info // TODO: store at the VertexLayout struct instead of re-creating for each render pass
    VkPipelineVertexInputStateCreateInfo           vertexInputInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkVertexInputBindingDescription                bindingDescription{};
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(vertexLayout.vertexLayoutAttributes.size());
    {
        if (vertexLayout.vertexLayoutAttributes.size() == 0) {
            vertexInputInfo.vertexBindingDescriptionCount   = 0;
            vertexInputInfo.vertexAttributeDescriptionCount = 0;
            vertexInputInfo.pVertexBindingDescriptions      = nullptr;
            vertexInputInfo.pVertexAttributeDescriptions    = nullptr;
        } else {
            for (int i = 0; i < vertexLayout.vertexLayoutAttributes.size(); i++) {
                auto& vkAttributeDescription = attributeDescriptions[i];
                auto& attribute              = vertexLayout.vertexLayoutAttributes[i];

                vkAttributeDescription.binding  = 0;
                vkAttributeDescription.location = i;
                vkAttributeDescription.format   = (VkFormat)attribute.format; // TODO: do not cast, use conversion util
                vkAttributeDescription.offset   = attribute.offset;
            }
            bindingDescription.binding   = 0;
            bindingDescription.stride    = vertexLayout.vertexSize;
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            vertexInputInfo.vertexBindingDescriptionCount   = 1;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
            vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
            vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();
        }
    }

    // create graphics pipeline //
    VkPipeline vkGraphicsPipeline;
    OZ_VK_ASSERT(ivkCreateGraphicsPipeline(m_device,
                                         (VkPipelineShaderStageCreateInfo[]){vertexShader->vkPipelineShaderStageCreateInfo,
                                                                             fragmentShader->vkPipelineShaderStageCreateInfo},
                                         2,
                                         window->vkSwapChainExtent,
                                         vkPipelineLayout,
                                         vkRenderPass,
                                         &vertexInputInfo,
                                         &vkGraphicsPipeline));

    // create frame buffers //
    std::vector<VkFramebuffer> vkFrameBuffers(window->vkSwapChainImageViews.size());
    for (size_t i = 0; i < window->vkSwapChainImageViews.size(); i++) {
        OZ_VK_ASSERT(ivkCreateFramebuffer(m_device, vkRenderPass, window->vkSwapChainExtent, window->vkSwapChainImageViews[i], &vkFrameBuffers[i]));
    }

    // create render pass object //
    RenderPass renderPass          = OZ_CREATE_VK_OBJECT(RenderPass);
    renderPass->vkRenderPass       = vkRenderPass;
    renderPass->vkPipelineLayout   = vkPipelineLayout;
    renderPass->vkGraphicsPipeline = vkGraphicsPipeline;
    renderPass->vkExtent           = window->vkSwapChainExtent;
    renderPass->vkFrameBuffers     = std::move(vkFrameBuffers);

    return renderPass;
}

Semaphore GraphicsDevice::createSemaphore() {
    // create semaphore //
    VkSemaphore vkSemaphore;
    OZ_VK_ASSERT(ivkCreateSemaphore(m_device, &vkSemaphore));

    // create semaphore object //
    Semaphore semaphore    = OZ_CREATE_VK_OBJECT(Semaphore);
    semaphore->vkSemaphore = vkSemaphore;

    return semaphore;
}

Buffer GraphicsDevice::createBuffer(BufferType bufferType, uint64_t size, const void* data) {
    // init buffer info and buffer flags //
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkBufferCreateInfo    bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = 0;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    switch (bufferType) {
    case BufferType::Vertex:
        bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    case BufferType::Index:
        bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        break;
    case BufferType::Staging:
        bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    case BufferType::Uniform:
        bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    default:
        throw std::runtime_error("Not supported buffer type!");
        break;
    }

    // create buffer //
    VkBuffer vkBuffer;
    OZ_VK_ASSERT(vkCreateBuffer(m_device, &bufferInfo, nullptr, &vkBuffer));

    // find suitable memory and allocate //
    VkDeviceMemory vkBufferMemory;
    OZ_VK_ASSERT(ivkAllocateMemory(m_device, m_physicalDevice, vkBuffer, properties, &vkBufferMemory));

    // bind memory //
    vkBindBufferMemory(m_device, vkBuffer, vkBufferMemory, 0);

    // copy the data to the buffer memory //
    if (data != nullptr) {
        void* pData;
        vkMapMemory(m_device, vkBufferMemory, 0, size, 0, &pData);
        memcpy(pData, data, (size_t)size);
        vkUnmapMemory(m_device, vkBufferMemory);
    }

    // create buffer object //
    Buffer buffer    = OZ_CREATE_VK_OBJECT(Buffer);
    buffer->vkBuffer = vkBuffer;
    buffer->vkMemory = vkBufferMemory;

    return buffer;
}

void GraphicsDevice::waitIdle() const { vkDeviceWaitIdle(m_device); }

Fence GraphicsDevice::createFence() {
    // create fence //
    VkFence vkFence;
    OZ_VK_ASSERT(ivkCreateFence(m_device, &vkFence));

    // create fence object //
    Fence fence    = OZ_CREATE_VK_OBJECT(Fence);
    fence->vkFence = vkFence;

    return fence;
}

void GraphicsDevice::waitFences(Fence fence, uint32_t fenceCount, bool waitAll) const {
    vkWaitForFences(m_device, fenceCount, &fence->vkFence, waitAll ? VK_TRUE : VK_FALSE, UINT64_MAX);
}

void GraphicsDevice::resetFences(Fence fence, uint32_t fenceCount) const { vkResetFences(m_device, fenceCount, &fence->vkFence); }

CommandBuffer GraphicsDevice::getCurrentCommandBuffer() { return m_commandBuffers[m_currentFrame]; }

uint32_t GraphicsDevice::getCurrentImage(Window window) const {
    glfwPollEvents();

    waitFences(m_inFlightFences[m_currentFrame], 1);
    resetFences(m_inFlightFences[m_currentFrame], 1);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_device,
                          window->vkSwapChain,
                          UINT64_MAX,
                          m_imageAvailableSemaphores[m_currentFrame]->vkSemaphore,
                          VK_NULL_HANDLE,
                          &imageIndex);

    return imageIndex;
}

bool GraphicsDevice::isWindowOpen(Window window) const { return !glfwWindowShouldClose(window->vkWindow); }

void GraphicsDevice::presentImage(Window window, uint32_t imageIndex) {
    OZ_VK_ASSERT(ivkQueuePresent(window->vkPresentQueue, m_renderFinishedSemaphores[m_currentFrame]->vkSemaphore, window->vkSwapChain, imageIndex));

    m_currentFrame = (m_currentFrame + 1) % FRAMES_IN_FLIGHT;
}

void GraphicsDevice::beginCmd(CommandBuffer cmd, bool isSingleUse) const {
    vkResetCommandBuffer(cmd->vkCommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = isSingleUse ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0;
    beginInfo.pInheritanceInfo = nullptr; // optional

    OZ_VK_ASSERT(vkBeginCommandBuffer(cmd->vkCommandBuffer, &beginInfo));
}

void GraphicsDevice::endCmd(CommandBuffer cmd) const { OZ_VK_ASSERT(vkEndCommandBuffer(cmd->vkCommandBuffer)); }

void GraphicsDevice::submitCmd(CommandBuffer cmd) const {
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    OZ_VK_ASSERT(ivkQueueSubmit(m_graphicsQueue,
                               m_imageAvailableSemaphores[m_currentFrame]->vkSemaphore,
                               waitStage,
                               cmd->vkCommandBuffer,
                               m_renderFinishedSemaphores[m_currentFrame]->vkSemaphore,
                               m_inFlightFences[m_currentFrame]->vkFence));
}

void GraphicsDevice::beginRenderPass(CommandBuffer cmd, RenderPass renderPass, uint32_t imageIndex) const {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass->vkRenderPass;
    renderPassInfo.framebuffer       = renderPass->vkFrameBuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = renderPass->vkExtent;
    VkClearValue clearColor          = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount   = 1;
    renderPassInfo.pClearValues      = &clearColor;

    vkCmdBeginRenderPass(cmd->vkCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd->vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass->vkGraphicsPipeline);

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(renderPass->vkExtent.width);
    viewport.height   = static_cast<float>(renderPass->vkExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd->vkCommandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = renderPass->vkExtent;
    vkCmdSetScissor(cmd->vkCommandBuffer, 0, 1, &scissor);
}

void GraphicsDevice::endRenderPass(CommandBuffer cmd) const { vkCmdEndRenderPass(cmd->vkCommandBuffer); }

void GraphicsDevice::draw(CommandBuffer cmd, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) const {
    vkCmdDraw(cmd->vkCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void GraphicsDevice::drawIndexed(
    CommandBuffer cmd, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance) const {
    vkCmdDrawIndexed(cmd->vkCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void GraphicsDevice::bindVertexBuffer(CommandBuffer cmd, Buffer vertexBuffer) {
    vkCmdBindVertexBuffers(cmd->vkCommandBuffer, 0, 1, (VkBuffer[]){vertexBuffer->vkBuffer}, (VkDeviceSize[]){0});
}

void GraphicsDevice::bindIndexBuffer(CommandBuffer cmd, Buffer indexBuffer) {
    vkCmdBindIndexBuffer(cmd->vkCommandBuffer, indexBuffer->vkBuffer, 0, VK_INDEX_TYPE_UINT16);
}

void GraphicsDevice::copyBuffer(Buffer src, Buffer dst, uint64_t size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = m_commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size      = size;
    vkCmdCopyBuffer(commandBuffer, src->vkBuffer, dst->vkBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_graphicsQueue);

    vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
}

void GraphicsDevice::free(Window window) const { OZ_FREE_VK_OBJECT(m_device, window); }
void GraphicsDevice::free(Shader shader) const { OZ_FREE_VK_OBJECT(m_device, shader); }
void GraphicsDevice::free(RenderPass renderPass) const { OZ_FREE_VK_OBJECT(m_device, renderPass) }
void GraphicsDevice::free(Semaphore semaphore) const { OZ_FREE_VK_OBJECT(m_device, semaphore); }
void GraphicsDevice::free(Fence fence) const { OZ_FREE_VK_OBJECT(m_device, fence); }
void GraphicsDevice::free(CommandBuffer commandBuffer) const { OZ_FREE_VK_OBJECT(m_device, commandBuffer); }
void GraphicsDevice::free(Buffer buffer) const { OZ_FREE_VK_OBJECT(m_device, buffer); }

} // namespace oz::gfx::vk