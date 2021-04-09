#include "includes/graphics.h"

bool Graphics::createSwapChain() {
    if (this->device == nullptr) return false;

    const std::vector<VkPresentModeKHR> presentModes = this->queryDeviceSwapModes();
    if (presentModes.empty()) {
        std::cerr << "Swap Modes Require Surface!" << std::endl;
        return false;
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (!this->getSurfaceCapabilities(surfaceCapabilities)) return false;

    if (!this->getSwapChainExtent(surfaceCapabilities)) {
        std::cerr << "Failed to get Swap Extent!" << std::endl;
        return false;
    }

    uint32_t imageCount = 3;
    if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.surface = this->vkSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = this->swapChainImageFormat.format;
    createInfo.imageColorSpace = this->swapChainImageFormat.colorSpace;
    createInfo.imageExtent = this->swapChainExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.presentMode = this->pickBestDeviceSwapMode(presentModes);
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    const uint32_t queueFamilyIndices[] = { this->graphicsQueueIndex, this->presentQueueIndex };
    if (this->graphicsQueueIndex != this->presentQueueIndex) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = imageCount;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    VkResult ret = vkCreateSwapchainKHR(this->device, &createInfo, nullptr, &this->swapChain);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Creat Swap Chain!" << std::endl;
        return false;
    }

    ret = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Get Swap Chain Image Count!" << std::endl;
        return false;
    }

    std::cout << "Buffering: " << imageCount << std::endl;
    this->swapChainImages.resize(imageCount);

    ret = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, this->swapChainImages.data());
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Swap Chain Images!" << std::endl;
        return false;
    }

    return true;
}

bool Graphics::createImageViews() {
    this->swapChainImageViews.resize(this->swapChainImages.size());

    for (size_t i = 0; i < this->swapChainImages.size(); i++) {
        VkImageView imgView = 
            this->createImageView(this->swapChainImages[i], this->swapChainImageFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
        if (imgView == nullptr) {
            std::cerr << "Failed to Create Swap Chain Image View!" << std::endl;
            return false;
        }
        
        this->swapChainImageViews[i] = imgView;
    }

    return true;
}

bool Graphics::createGraphicsPipeline() {
    if (this->vertexBuffer == nullptr) return true;
    
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    const VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    const std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) this->swapChainExtent.width;
    viewport.height = (float) this->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = this->swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = this->showWireFrame ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = 
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.blendEnable = VK_TRUE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
        
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(struct ModelProperties);
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pSetLayouts = &this->descriptorSetLayout;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VkResult ret = vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &this->graphicsPipelineLayout);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Pipeline Layout!" << std::endl;
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(this->shaderStageInfo.size());
    pipelineInfo.pStages = this->shaderStageInfo.data();
    pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = this->graphicsPipelineLayout;
    pipelineInfo.renderPass = this->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    ret = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->graphicsPipeline);
    ASSERT_VULKAN(ret);

    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Graphics Pipeline!" << std::endl;
        return false;
    }

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "createGraphicsPipeline: " << time_span.count() <<  std::endl;

    return true;
}

bool Graphics::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = this->swapChainImageFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkFormat depthFormat;
    if (!this->findDepthFormat(depthFormat)) {
        std::cerr << "Failed to Find Depth Format!" << std::endl;
        return false;
    }
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult ret = vkCreateRenderPass(this->device, &renderPassInfo, nullptr, &this->renderPass);
    ASSERT_VULKAN(ret);

    if (ret != VK_SUCCESS) {
       std::cerr << "Failed to Create Render Pass!" << std::endl;
       return false;
    }

    return true;
}

bool Graphics::createLogicalDeviceAndQueues() {
    // pick best physical device and queue index
    const std::tuple<VkPhysicalDevice, int> & bestPhysicalDeviceAndQueue = this->pickBestPhysicalDeviceAndQueueIndex();

    this->physicalDevice = std::get<0>(bestPhysicalDeviceAndQueue);
    // could be that we don't have a graphics queue (unlikely but whatever)
    if (this->physicalDevice == nullptr) {
        std::cerr << "No adequate Physical Device found" << std::endl;
        return false;
    }

    const int bestPhysicalQueueIndex = std::get<1>(bestPhysicalDeviceAndQueue);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    VkDeviceQueueCreateInfo queueCreateInfo;
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.flags = 0;
    queueCreateInfo.pNext = nullptr;
    queueCreateInfo.queueFamilyIndex = bestPhysicalQueueIndex;
    queueCreateInfo.queueCount = 2;
    const float priority = 1.0f;
    queueCreateInfo.pQueuePriorities = &priority;

    queueCreateInfos.push_back(queueCreateInfo);

    const std::vector<const char * > extensionsToEnable = { 
        "VK_KHR_swapchain"
    };

    VkPhysicalDeviceFeatures deviceFeatures {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.multiDrawIndirect = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;

    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.ppEnabledExtensionNames = extensionsToEnable.data();
    createInfo.enabledExtensionCount = extensionsToEnable.size();

    const VkResult ret = vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to create Logical Device!" << std::endl;
        return false;
    }

    this->graphicsQueueIndex = this->presentQueueIndex = queueCreateInfos[0].queueFamilyIndex;

    vkGetDeviceQueue(this->device, this->graphicsQueueIndex , 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->device, this->presentQueueIndex , 0, &this->presentQueue);

    return true;
}

void Graphics::createVkInstance(const std::string & appName, uint32_t version) {
    VkApplicationInfo app;
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pNext = nullptr,
    app.pApplicationName = appName.c_str();
    app.applicationVersion = version;
    app.pEngineName = appName.c_str();
    app.engineVersion = version;
    app.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo inst_info;
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = nullptr;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app;
    inst_info.enabledLayerCount = this->vkLayerNames.size();
    inst_info.ppEnabledLayerNames = !this->vkLayerNames.empty() ? this->vkLayerNames.data() : nullptr;
    inst_info.enabledExtensionCount = this->vkExtensionNames.size();
    inst_info.ppEnabledExtensionNames = !this->vkExtensionNames.empty() ? this->vkExtensionNames.data() : nullptr;

    const VkResult ret = vkCreateInstance(&inst_info, nullptr, &this->vkInstance);
    ASSERT_VULKAN(ret);
}

bool Graphics::initVulkan(const std::string & appName, uint32_t version) {
    this->queryVkInstanceExtensions();

    this->createVkInstance(appName, version);
    if (this->vkInstance == nullptr) return false;

    this->queryPhysicalDevices();

    if (this->vkPhysicalDevices.empty()) {
        std::cerr << "No Physical Devices Found" << std::endl;
        return false;
    }


    if (!SDL_Vulkan_CreateSurface(this->sdlWindow, this->vkInstance, &this->vkSurface)) {
        std::cerr << "SDL Vulkan Surface could not be created! Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (!this->createLogicalDeviceAndQueues()) return false;

    if (!this->createSwapChain()) return false;
    if (!this->createImageViews()) return false;
    if (!this->createRenderPass()) return false;
    if (!this->createCommandPool()) return false;
    
    this->hasTerrain = this->createTerrain();
    this->hasSkybox = this->createSkybox();
        
    if (!this->createShaderStageInfo()) return false;
    if (this->hasSkybox) {
        if (!this->createSkyboxShaderStageInfo()) return false;        
        if (!this->createSkyboxDescriptorPool()) return false;
    }
    if (this->hasTerrain) {
        if (!this->createTerrainShaderStageInfo()) return false;        
        if (!this->createTerrainDescriptorPool()) return false;
    }
    
    if (!this->createDescriptorPool()) return false;
    
    if (!this->createSyncObjects()) return false;
    
    return true;
}

bool Graphics::createSyncObjects() {
    this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    this->imagesInFlight.resize(this->swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        if (vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(this->device, &fenceInfo, nullptr, &this->inFlightFences[i]) != VK_SUCCESS) {
            std::cerr << "Failed to Create Synchronization Objects For Frame!" << std::endl;
            return false;
        }
    }

    return true;
}

bool Graphics::createCommandPool() {
    if (this->device == nullptr) return false;
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolInfo.queueFamilyIndex = this->graphicsQueueIndex;

    VkResult ret = vkCreateCommandPool(this->device, &poolInfo, nullptr, &this->commandPool);
    ASSERT_VULKAN(ret);

    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Command Pool" << std::endl;
        return false;
    }

    return true;
}

bool Graphics::createUniformBuffers() {
    if (!this->createTextureSampler(this->textureSampler, VK_SAMPLER_ADDRESS_MODE_REPEAT)) return false;
    
    if (!this->createDescriptorSetLayout()) return false;
    
    VkDeviceSize bufferSize = sizeof(struct ModelUniforms);

    this->uniformBuffers.resize(this->swapChainImages.size());
    this->uniformBuffersMemory.resize(this->swapChainImages.size());

    for (size_t i = 0; i < this->swapChainImages.size(); i++) {
        this->createBuffer(
            bufferSize, 
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            this->uniformBuffers[i], this->uniformBuffersMemory[i]);
    }
    
    if (!this->createDescriptorSets()) return false;

    if (this->hasTerrain) {
        if (!this->createTerrainDescriptorSetLayout()) return false;
        if (!this->createTerrainDescriptorSets()) return false;
    }
    
    if (this->hasSkybox) {
        if (!this->createTextureSampler(this->skyboxSampler, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)) return false;
        if (!this->createSkyboxDescriptorSetLayout()) return false;
        if (!this->createSkyboxDescriptorSets()) return false;
    }

    return true;
}

bool Graphics::createDepthResources() {
    this->depthImages.resize(this->swapChainImages.size());
    this->depthImagesMemory.resize(this->depthImages.size());
    this->depthImagesView.resize(this->depthImages.size());
    
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    if (!this->findDepthFormat(depthFormat)) {
    
        return false;
    };

    for (uint16_t i=0;i<this->depthImages.size();i++) {
        if (!this->createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->depthImages[i], this->depthImagesMemory[i])) {
            std::cerr << "Faild to create Depth Image!" << std::endl;
            return false;
        }
    
        this->depthImagesView[i] = this->createImageView(this->depthImages[i], depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
        if (this->depthImagesView[i] == nullptr) {
            std::cerr << "Faild to create Depth Image View!" << std::endl;
            return false;        
        }
    }
    
    return true;
}

bool Graphics::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 3> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(this->swapChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(this->swapChainImages.size());
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_TEXTURES * this->swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

    VkResult ret = vkCreateDescriptorPool(device, &poolInfo, nullptr, &this->descriptorPool);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
       std::cerr << "Failed to Create Descriptor Pool!" << std::endl;
       return false;
    }
    
    return true;
}

bool Graphics::createFramebuffers() {
     this->swapChainFramebuffers.resize(this->swapChainImageViews.size());

     for (size_t i = 0; i < this->swapChainImageViews.size(); i++) {
         std::array<VkImageView, 2> attachments = {
             this->swapChainImageViews[i], this->depthImagesView[i]
         };

         VkFramebufferCreateInfo framebufferInfo{};
         framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
         framebufferInfo.renderPass = renderPass;
         framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
         framebufferInfo.pAttachments = attachments.data();
         framebufferInfo.width = swapChainExtent.width;
         framebufferInfo.height = swapChainExtent.height;
         framebufferInfo.layers = 1;

         VkResult ret = vkCreateFramebuffer(this->device, &framebufferInfo, nullptr, &this->swapChainFramebuffers[i]);
         ASSERT_VULKAN(ret);

         if (ret != VK_SUCCESS) {
            std::cerr << "Failed to Create Frame Buffer!" << std::endl;
            return false;
         }
     }

     return true;
}

VkShaderModule Graphics::createShaderModule(const std::vector<char> & code) {
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult ret = vkCreateShaderModule(this->device, &createInfo, nullptr, &shaderModule);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Shader Module!" << std::endl;
        return nullptr;
    }

    return shaderModule;
}

bool Graphics::createShaderStageInfo() {
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    if (!Utils::readFile(this->getAppPath(SHADERS) / "vert.spv", vertShaderCode) ||
            !Utils::readFile(this->getAppPath(SHADERS) / "frag.spv", fragShaderCode)) {
        std::cerr << "Failed to read shader files: " << this->getAppPath(SHADERS) << std::endl;
        return false;
    }

    this->vertShaderModule = this->createShaderModule(vertShaderCode);
    if (vertShaderModule == nullptr) return false;
    this->fragShaderModule = this->createShaderModule(fragShaderCode);
    if (this->fragShaderModule == nullptr) return false;
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = this->vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = this->fragShaderModule;
    fragShaderStageInfo.pName = "main";

    this->shaderStageInfo[0] = vertShaderStageInfo;
    this->shaderStageInfo[1] = fragShaderStageInfo;
    
    return true;
}

bool Graphics::createDescriptorSetLayout() {
    if (this->vertexBuffer == nullptr) return true;
    
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    VkDescriptorSetLayoutBinding modelUniformLayoutBinding{};
    modelUniformLayoutBinding.binding = 0;
    modelUniformLayoutBinding.descriptorCount = 1;
    modelUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelUniformLayoutBinding.pImmutableSamplers = nullptr;
    modelUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings.push_back(modelUniformLayoutBinding);

    VkDescriptorSetLayoutBinding ssboLayoutBinding{};
    ssboLayoutBinding.binding = 1;
    ssboLayoutBinding.descriptorCount = 1;
    ssboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssboLayoutBinding.pImmutableSamplers = nullptr;
    ssboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings.push_back(ssboLayoutBinding);

    uint32_t numberOfTextures = this->models.getTextures().size();
    VkDescriptorSetLayoutBinding samplersLayoutBinding{};
    samplersLayoutBinding.binding = 2;
    samplersLayoutBinding.descriptorCount = numberOfTextures > 0 ? numberOfTextures : 0;
    samplersLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplersLayoutBinding.pImmutableSamplers = nullptr;
    samplersLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings.push_back(samplersLayoutBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = layoutBindings.size();
    layoutInfo.pBindings = layoutBindings.data();

    VkResult ret = vkCreateDescriptorSetLayout(this->device, &layoutInfo, nullptr, &this->descriptorSetLayout);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Descriptor Set Layout!" << std::endl;
        return false;
    }
    
    return true;
}

bool Graphics::createDescriptorSets() {
    if (this->vertexBuffer == nullptr) return true;
    
    std::vector<VkDescriptorSetLayout> layouts(this->swapChainImages.size(), this->descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(this->swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    this->descriptorSets.resize(this->swapChainImages.size());
    VkResult ret = vkAllocateDescriptorSets(this->device, &allocInfo, this->descriptorSets.data());
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Allocate Descriptor Sets!" << std::endl;
        return false;
    }

    std::map<std::string, std::unique_ptr<Texture>> & textures = this->models.getTextures();
    uint32_t numberOfTextures = textures.size();        

    std::vector<VkDescriptorImageInfo> descriptorImageInfos;
    if (numberOfTextures > 0) {
        for (uint32_t i = 0; i < numberOfTextures; ++i) {
            VkDescriptorImageInfo texureDescriptorInfo = {};
            texureDescriptorInfo.sampler = this->textureSampler;
            texureDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            texureDescriptorInfo.imageView = this->models.findTextureImageViewById(i);
            descriptorImageInfos.push_back(texureDescriptorInfo);
        }
    } else {
            VkDescriptorImageInfo texureDescriptorInfo = {};
            texureDescriptorInfo.sampler = nullptr;
            texureDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            texureDescriptorInfo.imageView = this->models.findTextureImageViewById(-1);
            descriptorImageInfos.push_back(texureDescriptorInfo);        
    }

    VkDescriptorBufferInfo ssboBufferInfo{};
    ssboBufferInfo.buffer = this->ssboBuffer;
    ssboBufferInfo.offset = 0;
    ssboBufferInfo.range = this->getModelsBufferSizes().ssboBufferSize;

    for (size_t i = 0; i < this->descriptorSets.size(); i++) {
        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = this->uniformBuffers[i];
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = sizeof(struct ModelUniforms);

        std::vector<VkWriteDescriptorSet> descriptorWrites;

        VkWriteDescriptorSet uniformDescriptorSet = {};
        uniformDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformDescriptorSet.dstSet = this->descriptorSets[i];
        uniformDescriptorSet.dstBinding = 0;
        uniformDescriptorSet.dstArrayElement = 0;
        uniformDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformDescriptorSet.descriptorCount = 1;
        uniformDescriptorSet.pBufferInfo = &uniformBufferInfo;
        descriptorWrites.push_back(uniformDescriptorSet);

        VkWriteDescriptorSet ssboDescriptorSet = {};
        ssboDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        ssboDescriptorSet.dstSet = this->descriptorSets[i];
        ssboDescriptorSet.dstBinding = 1;
        ssboDescriptorSet.dstArrayElement = 0;
        ssboDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        ssboDescriptorSet.descriptorCount = 1;
        ssboDescriptorSet.pBufferInfo = &ssboBufferInfo;
        descriptorWrites.push_back(ssboDescriptorSet);

        VkWriteDescriptorSet samplerDescriptorSet = {};
        samplerDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerDescriptorSet.dstBinding = 2;
        samplerDescriptorSet.dstArrayElement = 0;
        samplerDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerDescriptorSet.descriptorCount = numberOfTextures > 0 ? numberOfTextures : 1;
        samplerDescriptorSet.pImageInfo = descriptorImageInfos.data();
        samplerDescriptorSet.dstSet = this->descriptorSets[i];
        descriptorWrites.push_back(samplerDescriptorSet);

        vkUpdateDescriptorSets(this->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    
    return true;
}
