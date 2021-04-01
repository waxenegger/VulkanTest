#include "includes/graphics.h"

bool Graphics::createSkyboxDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(this->swapChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(this->swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

    VkResult ret = vkCreateDescriptorPool(device, &poolInfo, nullptr, &this->skyboxDescriptorPool);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
       std::cerr << "Failed to Create Skybox Descriptor Pool!" << std::endl;
       return false;
    }
    
    return true;
}

bool Graphics::createSkyboxDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    VkDescriptorSetLayoutBinding modelUniformLayoutBinding{};
    modelUniformLayoutBinding.binding = 0;
    modelUniformLayoutBinding.descriptorCount = 1;
    modelUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelUniformLayoutBinding.pImmutableSamplers = nullptr;
    modelUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings.push_back(modelUniformLayoutBinding);

    VkDescriptorSetLayoutBinding samplersLayoutBinding{};
    samplersLayoutBinding.binding = 1;
    samplersLayoutBinding.descriptorCount = 1;
    samplersLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplersLayoutBinding.pImmutableSamplers = nullptr;
    samplersLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings.push_back(samplersLayoutBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = layoutBindings.size();
    layoutInfo.pBindings = layoutBindings.data();

    VkResult ret = vkCreateDescriptorSetLayout(this->device, &layoutInfo, nullptr, &this->skyboxDescriptorSetLayout);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Descriptor Set Layout!" << std::endl;
        return false;
    }
    
    return true;
}

bool Graphics::createSkyboxDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(this->swapChainImages.size(), this->skyboxDescriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->skyboxDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(this->swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    this->skyboxDescriptorSets.resize(this->swapChainImages.size());
    VkResult ret = vkAllocateDescriptorSets(this->device, &allocInfo, this->skyboxDescriptorSets.data());
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Allocate Skybox Descriptor Sets!" << std::endl;
        return false;
    }

    VkDescriptorImageInfo descriptorImageInfo;
    descriptorImageInfo.sampler = this->skyboxSampler;
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo.imageView = this->skyboxImageView;

    for (size_t i = 0; i < this->skyboxDescriptorSets.size(); i++) {
        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = this->uniformBuffers[i];
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = sizeof(struct ModelUniforms);

        std::vector<VkWriteDescriptorSet> descriptorWrites;

        VkWriteDescriptorSet uniformDescriptorSet = {};
        uniformDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformDescriptorSet.dstSet = this->skyboxDescriptorSets[i];
        uniformDescriptorSet.dstBinding = 0;
        uniformDescriptorSet.dstArrayElement = 0;
        uniformDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformDescriptorSet.descriptorCount = 1;
        uniformDescriptorSet.pBufferInfo = &uniformBufferInfo;
        descriptorWrites.push_back(uniformDescriptorSet);
        
        VkWriteDescriptorSet samplerDescriptorSet = {};
        samplerDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        samplerDescriptorSet.dstBinding = 1;
        samplerDescriptorSet.dstArrayElement = 0;
        samplerDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerDescriptorSet.descriptorCount = 1;
        samplerDescriptorSet.pImageInfo = &descriptorImageInfo;
        samplerDescriptorSet.dstSet = this->skyboxDescriptorSets[i];
        descriptorWrites.push_back(samplerDescriptorSet);

        vkUpdateDescriptorSets(this->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    
    return true;
}

bool Graphics::createSkyboxShaderStageInfo() {
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    if (!Utils::readFile(this->getAppPath(SHADERS) / "skybox_vert.spv", vertShaderCode) ||
            !Utils::readFile(this->getAppPath(SHADERS) / "skybox_frag.spv", fragShaderCode)) {
        std::cerr << "Failed to read shader files: " << this->getAppPath(SHADERS) << std::endl;
        return false;
    }

    this->skyboxVertShaderModule = this->createShaderModule(vertShaderCode);
    if (skyboxVertShaderModule == nullptr) return false;
    this->skyboxFragShaderModule = this->createShaderModule(fragShaderCode);
    if (this->skyboxFragShaderModule == nullptr) return false;
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = this->skyboxVertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = this->skyboxFragShaderModule;
    fragShaderStageInfo.pName = "main";

    this->skyboxShaderStageInfo[0] = vertShaderStageInfo;
    this->skyboxShaderStageInfo[1] = fragShaderStageInfo;
    
    return true;
}

bool Graphics::createSkyboxGraphicsPipeline() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (this->skyBoxVertexBuffer != nullptr) {
        const VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
        const std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
        vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    } else {
        vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
        vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    }
    
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
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pSetLayouts = &this->skyboxDescriptorSetLayout;
    pipelineLayoutInfo.setLayoutCount = 1;

    VkResult ret = vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &this->skyboxGraphicsPipelineLayout);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Skybox Pipeline Layout!" << std::endl;
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(this->skyboxShaderStageInfo.size());
    pipelineInfo.pStages = this->skyboxShaderStageInfo.data();
    pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = this->skyboxGraphicsPipelineLayout;
    pipelineInfo.renderPass = this->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    ret = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->skyboxGraphicsPipeline);
    ASSERT_VULKAN(ret);

    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Skybox Graphics Pipeline!" << std::endl;
        return false;
    }

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "createSkyboxGraphicsPipeline: " << time_span.count() <<  std::endl;

    return true;
}

bool Graphics::createSkybox() {
    std::array<std::string, 6> skyboxCubeImageLocations = {
        "sky_right.png", "sky_left.png", "sky_top.png", "sky_bottom.png", "sky_front.png", "sky_back.png" 
    };
    std::vector<std::unique_ptr<Texture>> skyboxCubeTextures;

    for (auto & s : skyboxCubeImageLocations) {
        std::unique_ptr<Texture> texture = std::make_unique<Texture>();
        texture->setPath(this->getAppPath(MODELS) / s);
        texture->load();
        if (texture->isValid()) {
            skyboxCubeTextures.push_back(std::move(texture));
        }            
    }
    
    if (skyboxCubeTextures.size() != 6) return false;

    if (this->skyBoxVertexBuffer != nullptr) vkDestroyBuffer(this->device, this->skyBoxVertexBuffer, nullptr);
    if (this->skyBoxVertexBufferMemory != nullptr) vkFreeMemory(this->device, this->skyBoxVertexBufferMemory, nullptr);

    VkDeviceSize bufferSize = SKYBOX_VERTICES.size() * sizeof(class Vertex);
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    if (!this->createBuffer(bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory)) {
        std::cerr << "Failed to get Create Skybox Staging Buffer" << std::endl;
        return false;
    }

    void* data = nullptr;
    vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, SKYBOX_VERTICES.data(), bufferSize);
    vkUnmapMemory(this->device, stagingBufferMemory);

    if (!this->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->skyBoxVertexBuffer, this->skyBoxVertexBufferMemory)) {
        std::cerr << "Failed to get Create Skybox Vertex Buffer" << std::endl;
        return false;
    }

    this->copyBuffer(stagingBuffer,this->skyBoxVertexBuffer, bufferSize);

    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);
    
    stagingBuffer = nullptr;
    stagingBufferMemory = nullptr;
    VkDeviceSize skyboxCubeSize = skyboxCubeTextures.size() * skyboxCubeTextures[0]->getSize();
    
    if (!this->createBuffer(
        skyboxCubeSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory)) {
            std::cerr << "Failed to Create Skybox Staging Buffer" << std::endl;
            return false;
    }

    data = nullptr;
    VkDeviceSize offset = 0;
    vkMapMemory(device, stagingBufferMemory, 0, skyboxCubeSize, 0, &data);
    for (auto & tex : skyboxCubeTextures) {
        memcpy(static_cast<char *>(data) + offset, tex->getPixels(), tex->getSize());
        offset += tex->getSize();
    }
    vkUnmapMemory(device, stagingBufferMemory);

    if (!this->createImage(
        skyboxCubeTextures[0]->getWidth(), skyboxCubeTextures[0]->getHeight(), skyboxCubeTextures[0]->getImageFormat(), 
        VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        this->skyboxCubeImage, this->skyboxCubeImageMemory, skyboxCubeTextures.size())) {
            std::cerr << "Failed to Create Skybox Image" << std::endl;
            return false;
    }

    transitionImageLayout(
        this->skyboxCubeImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, skyboxCubeTextures.size());
    
    this->copyBufferToImage(
        stagingBuffer, this->skyboxCubeImage, skyboxCubeTextures[0]->getWidth(), skyboxCubeTextures[0]->getHeight(), skyboxCubeTextures.size());
    
    transitionImageLayout(
        this->skyboxCubeImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, skyboxCubeTextures.size());

    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);
    
    this->skyboxImageView = 
        this->createImageView(this->skyboxCubeImage, skyboxCubeTextures[0]->getImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT, skyboxCubeTextures.size());
    if (this->skyboxImageView == nullptr) {
        std::cerr << "Failed to Create Skybox Image View!" << std::endl;
        return false;
    }

    return true;
}

