#include "includes/graphics.h"

TerrainMap::~TerrainMap() {
    if (this->map != nullptr) SDL_FreeSurface(this->map);
}

float Terrain::getHeightForPoint(const int x, const int y) {
    const VkExtent2D extent = this->getExtent();
    const uint32_t adjustedX = x + extent.width / 2;
    const uint32_t adjustedY = y + extent.height / 2;
    
    if (!this->hasBeenLoaded() || adjustedX > extent.width || adjustedY > extent.width) {
        return 0.0f;
    }
    
    uint64_t index = adjustedY * extent.width + adjustedX;
    if (index > this->terrainVertices.size()) {
        std::cerr << "Accessing Point beyond Terrain Size!" << std::endl;
    }
        
    return this->terrainVertices[index].getPosition().y;
}

std::vector<ColorVertex> & Terrain::getVertices() {
    return this->terrainVertices;
}

std::vector<uint32_t> & Terrain::getIndices() {
    return this->terrainIndices;
}

VkExtent2D TerrainMap::getExtent() {
    return { this->width * this->xFactor, this->height * this->yFactor };
}

TerrainMap::TerrainMap(const std::string & file, const uint8_t magnificationFactor) {
    this->map = IMG_Load(file.c_str());
    this->generateTerrain(magnificationFactor);
}

bool TerrainMap::hasBeenLoaded() {
    return this->loaded;
}

void TerrainMap::generateTerrain(const uint8_t magnificationFactor) {
    if (this->map == nullptr) return;

    this->width = this->map->w;
    this->height = this->map->h;

    this->xFactor = magnificationFactor;
    this->yFactor = magnificationFactor;
    
    uint16_t xRange = this->width * this->xFactor;
    uint16_t yRange = this->height * this->yFactor;
    
    uint64_t index = 0;
    uint64_t maxIndex = this->height * this->width * 4; 
    Uint8 * data = static_cast<Uint8 *>(this->map->pixels);
    
    uint16_t x = 0;
    uint16_t y = 0;

    std::vector<std::vector<ColorVertex>> additionalYfactorVertices;

    while(index < maxIndex) {
        const glm::vec4 pointData = {
            static_cast<float>(data[index]) / 255.0f,
            static_cast<float>(data[index+1]) / 255.0f,
            static_cast<float>(data[index+2]) / 255.0f,
            0
        };
        float height = 1+(1-pointData.b) * 25;
        
        for (uint8_t offsetY=0;offsetY<yFactor;offsetY++) {
            for (uint8_t offsetX=0;offsetX<xFactor;offsetX++) {
                ColorVertex v = ColorVertex(glm::vec3((x-xRange/2)+offsetX, height, (y-yRange/2)+offsetY));            
                v.setColor(glm::vec3(pointData.r, pointData.g, pointData.b));
                
                if (yFactor > 1 && offsetY > 0) {
                    if (additionalYfactorVertices.size() < offsetY) {
                        additionalYfactorVertices.push_back(std::vector<ColorVertex>());
                    }
                    additionalYfactorVertices[offsetY-1].push_back(v);
                } else this->terrainVertices.push_back(v);
            }
        }
        
        index += 4;
        x+=xFactor;
        
        if (x >= xRange) {
            if (!additionalYfactorVertices.empty()) {
                for(auto & vec : additionalYfactorVertices) {
                    for(auto & add : vec) this->terrainVertices.push_back(add);   
                }
                additionalYfactorVertices.clear();
            };
            
            x = 0;
            y+=yFactor;
        }
    }
    
    for (y=0;y<yRange;y++) {
        uint32_t yOff = y * xRange;
        for (x=0;x<xRange;x++) {
            if (y+1 >= yRange) break;
            
            uint32_t vIndex = yOff + x; 
                        
            // upper right triangles
            if (x>0) {
                this->terrainIndices.push_back(vIndex - 1);
                this->terrainIndices.push_back(vIndex + xRange);                
                this->terrainIndices.push_back(vIndex);
            }

            // lower left triangles            
            if (x+1 < xRange) {
                this->terrainIndices.push_back(vIndex);
                this->terrainIndices.push_back(vIndex + xRange);
                this->terrainIndices.push_back(vIndex + xRange + 1);
            }
            
            // normals
            float heightLeftNeighbor = 
                (x <= 0) ? this->getVertices()[vIndex].getPosition().y:
                    this->getVertices()[vIndex-1].getPosition().y;
                    
            float heightRightNeighbor = 
                (x+1 >= xRange) ? this->getVertices()[vIndex].getPosition().y :
                    this->getVertices()[vIndex+1].getPosition().y;
            
            float heightUpperNeighbor = 
                (y <= 0) ? this->getVertices()[vIndex].getPosition().y :
                    this->getVertices()[vIndex-xRange].getPosition().y;
                    
            float heightLowerNeighbor = 
                (y+1 >= yRange) ? this->getVertices()[vIndex].getPosition().y :
                    this->getVertices()[vIndex+xRange].getPosition().y;

            this->getVertices()[vIndex].setNormal(
                glm::normalize(glm::vec3(
                    heightLeftNeighbor - heightRightNeighbor,
                    2.0f,
                    heightUpperNeighbor - heightLowerNeighbor)
            ));
        }
    }
    
    if (this->map != nullptr) {
        SDL_FreeSurface(this->map);
        this->map = nullptr;
    }
    
    this->loaded = true;
}

BufferSummary Graphics::getTerrainBufferSizes() {
    BufferSummary bufferSizes;
    
    bufferSizes.vertexBufferSize = this->terrain->getVertices().size() * sizeof(class ColorVertex);
    bufferSizes.indexBufferSize = this->terrain->getIndices().size() * sizeof(uint32_t);

    std::cout << "Terrain Vertex Buffer Size: " << bufferSizes.vertexBufferSize / MEGA_BYTE << " MB" << std::endl;
    std::cout << "Terrain Index Buffer Size: " << bufferSizes.indexBufferSize / MEGA_BYTE << " MB" << std::endl;
    std::cout << "Terrain SSBO Buffer Size: " << bufferSizes.ssboBufferSize / MEGA_BYTE << " MB" << std::endl;
    
    return bufferSizes;
}

bool Graphics::createTerrainDescriptorPool() {
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

    VkResult ret = vkCreateDescriptorPool(device, &poolInfo, nullptr, &this->terrainDescriptorPool);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
       std::cerr << "Failed to Create Terrain Descriptor Pool!" << std::endl;
       return false;
    }
    
    return true;
}

bool Graphics::createTerrainDescriptorSetLayout() {
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

    VkDescriptorSetLayoutBinding modelUniformLayoutBinding{};
    modelUniformLayoutBinding.binding = 0;
    modelUniformLayoutBinding.descriptorCount = 1;
    modelUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelUniformLayoutBinding.pImmutableSamplers = nullptr;
    modelUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings.push_back(modelUniformLayoutBinding);

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = layoutBindings.size();
    layoutInfo.pBindings = layoutBindings.data();

    VkResult ret = vkCreateDescriptorSetLayout(this->device, &layoutInfo, nullptr, &this->terrainDescriptorSetLayout);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Terrain Descriptor Set Layout!" << std::endl;
        return false;
    }
    
    return true;
}

bool Graphics::createTerrainDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(this->swapChainImages.size(), this->terrainDescriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = this->terrainDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(this->swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    this->terrainDescriptorSets.resize(this->swapChainImages.size());
    VkResult ret = vkAllocateDescriptorSets(this->device, &allocInfo, this->terrainDescriptorSets.data());
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Allocate Terrain Descriptor Sets!" << std::endl;
        return false;
    }

    for (size_t i = 0; i < this->terrainDescriptorSets.size(); i++) {
        VkDescriptorBufferInfo uniformBufferInfo{};
        uniformBufferInfo.buffer = this->uniformBuffers[i];
        uniformBufferInfo.offset = 0;
        uniformBufferInfo.range = sizeof(struct ModelUniforms);

        std::vector<VkWriteDescriptorSet> descriptorWrites;

        VkWriteDescriptorSet uniformDescriptorSet = {};
        uniformDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        uniformDescriptorSet.dstSet = this->terrainDescriptorSets[i];
        uniformDescriptorSet.dstBinding = 0;
        uniformDescriptorSet.dstArrayElement = 0;
        uniformDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformDescriptorSet.descriptorCount = 1;
        uniformDescriptorSet.pBufferInfo = &uniformBufferInfo;
        descriptorWrites.push_back(uniformDescriptorSet);
        

        vkUpdateDescriptorSets(this->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    
    return true;
}

bool Graphics::createTerrainShaderStageInfo() {
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    if (!Utils::readFile(this->getAppPath(SHADERS) / "terrain_vert.spv", vertShaderCode) ||
            !Utils::readFile(this->getAppPath(SHADERS) / "terrain_frag.spv", fragShaderCode)) {
        std::cerr << "Failed to read shader files: " << this->getAppPath(SHADERS) << std::endl;
        return false;
    }

    this->terrainVertShaderModule = this->createShaderModule(vertShaderCode);
    if (this->terrainVertShaderModule == nullptr) return false;
    this->terrainFragShaderModule = this->createShaderModule(fragShaderCode);
    if (this->terrainFragShaderModule == nullptr) return false;
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = this->terrainVertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = this->terrainFragShaderModule;
    fragShaderStageInfo.pName = "main";

    this->terrainShaderStageInfo[0] = vertShaderStageInfo;
    this->terrainShaderStageInfo[1] = fragShaderStageInfo;
    
    return true;
}

bool Graphics::createTerrainGraphicsPipeline() {
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (this->terrainVertexBuffer != nullptr) {
        const VkVertexInputBindingDescription bindingDescription = ColorVertex::getBindingDescription();
        const std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions = ColorVertex::getAttributeDescriptions();

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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pSetLayouts = &this->terrainDescriptorSetLayout;
    pipelineLayoutInfo.setLayoutCount = 1;

    VkResult ret = vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &this->terrainGraphicsPipelineLayout);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Terrain Pipeline Layout!" << std::endl;
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(this->terrainShaderStageInfo.size());
    pipelineInfo.pStages = this->terrainShaderStageInfo.data();
    pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = this->terrainGraphicsPipelineLayout;
    pipelineInfo.renderPass = this->renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    ret = vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->terrainGraphicsPipeline);
    ASSERT_VULKAN(ret);

    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Terrain Graphics Pipeline!" << std::endl;
        return false;
    }

    return true;
}

bool Graphics::createTerrain() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    this->terrain = std::make_unique<TerrainMap>(this->getAppPath(MAPS) / "terrain.png", 2);
    if (!this->terrain->hasBeenLoaded()) return false;
    
    const BufferSummary bufferSizes = this->getTerrainBufferSizes();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    if (!this->createBuffer(bufferSizes.vertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory)) {
        std::cerr << "Failed to get Create Terrain Staging Buffer" << std::endl;
        return false;
    }

    void* data = nullptr;
    vkMapMemory(this->device, stagingBufferMemory, 0, bufferSizes.vertexBufferSize, 0, &data);
    memcpy(data, this->terrain->getVertices().data(), bufferSizes.vertexBufferSize);
    vkUnmapMemory(this->device, stagingBufferMemory);

    if (!this->createBuffer(
            bufferSizes.vertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->terrainVertexBuffer, this->terrainVertexBufferMemory)) {
        std::cerr << "Failed to get Create Terrain Vertex Buffer" << std::endl;
        return false;
    }

    this->copyBuffer(stagingBuffer,this->terrainVertexBuffer, bufferSizes.vertexBufferSize);

    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);

    if (bufferSizes.indexBufferSize > 0) {        
        if (!this->createBuffer(bufferSizes.indexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory)) {
            std::cerr << "Failed to get Create Staging Buffer" << std::endl;
            return false;
        }

        data = nullptr;
        vkMapMemory(this->device, stagingBufferMemory, 0, bufferSizes.indexBufferSize, 0, &data);
        memcpy(data, this->terrain->getIndices().data(), bufferSizes.indexBufferSize);
        vkUnmapMemory(this->device, stagingBufferMemory);

        if (!this->createBuffer(bufferSizes.indexBufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                this->terrainIndexBuffer, this->terrainIndexBufferMemory)) {
            std::cerr << "Failed to get Create Terrain Index Buffer" << std::endl;
            return false;
        }
        
        this->copyBuffer(stagingBuffer,this->terrainIndexBuffer, bufferSizes.indexBufferSize);
        
        vkDestroyBuffer(this->device, stagingBuffer, nullptr);
        vkFreeMemory(this->device, stagingBufferMemory, nullptr);
    }
    
    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "createTerrain: " << time_span.count() <<  std::endl;

    return true;
}

float Graphics::getTerrainHeightAtPosition(const glm::vec3 position) {
    return this->terrain->getHeightForPoint(-position.x, -position.z);
}
