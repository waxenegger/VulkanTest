#include "includes/graphics.h"

void Graphics::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

bool Graphics::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult ret = vkCreateBuffer(this->device, &bufferInfo, nullptr, &buffer);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to get Create Buffer" << std::endl;
        return false;
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(this->device, buffer, &memRequirements);

    uint32_t memoryTypeIndex;
    if (!this->findMemoryType(memRequirements.memoryTypeBits, properties,memoryTypeIndex)) {
        std::cerr << "Failed to get Memory Type Requested" << std::endl;
        return false;
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;


    ret = vkAllocateMemory(this->device, &allocInfo, nullptr, &bufferMemory);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to get Allocate Memory for Vertex Buffer" << std::endl;
        return false;
    }

    vkBindBufferMemory(this->device, buffer, bufferMemory, 0);

    return true;
}

bool Graphics::createImage(
    int32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint16_t arrayLayers) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = arrayLayers;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (arrayLayers > 1) {
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;    
        }
        
        VkResult ret = vkCreateImage(this->device, &imageInfo, nullptr, &image);
        ASSERT_VULKAN(ret);

        if (ret != VK_SUCCESS) {
            std::cerr << "Failed to Create Image" << std::endl;
            return false;
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        uint32_t memoryTypeIndex;
        if (!this->findMemoryType(memRequirements.memoryTypeBits, properties, memoryTypeIndex)) {
            std::cerr << "Failed to get Memory Type Requested" << std::endl;
            return false;            
        }

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        
        ret = vkAllocateMemory(this->device, &allocInfo, nullptr, &imageMemory);
        ASSERT_VULKAN(ret);

        if (ret != VK_SUCCESS) {
            std::cerr << "Failed to Allocate Image Memory" << std::endl;
            return false;
        }
        
        vkBindImageMemory(this->device, image, imageMemory, 0);
        
        return true;
}

VkCommandBuffer Graphics::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = this->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = nullptr;

    VkResult ret = vkAllocateCommandBuffers(this->device, &allocInfo, &commandBuffer);
    ASSERT_VULKAN(ret);

    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Allocate Command Buffer" << std::endl;
        return nullptr;
    }


    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    ret = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    ASSERT_VULKAN(ret);

    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Begin Command Buffer" << std::endl;
        return nullptr;
    }

    return commandBuffer;
}

void Graphics::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

bool Graphics::transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint16_t layerCount) {
    VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();
    if (commandBuffer == nullptr) return false;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;        
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == newLayout) {
        return true;
    } else
    {
        std::cerr << "Unsupported Layout Transition" << std::endl;
        return false;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    this->endSingleTimeCommands(commandBuffer);
    
    return true;
}

void Graphics::copyBufferToImage(VkBuffer & buffer, VkImage & image, uint32_t width, uint32_t height, uint16_t layerCount) {
    VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();
    if (commandBuffer == nullptr) return;

    std::vector<VkBufferImageCopy> regions;
    VkBufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = { width, height, 1};
    regions.push_back(region);

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());

    endSingleTimeCommands(commandBuffer);    
}

bool Graphics::createTextureSampler(VkSampler & sampler, VkSamplerAddressMode addressMode) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkResult ret = vkCreateSampler(this->device, &samplerInfo, nullptr, &sampler);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Texture Sampler!" << std::endl;
        return false;
    }

    return true;
}

void Graphics::copyModelsContentIntoBuffer(void* data, ModelsContentType modelsContentType, VkDeviceSize maxSize) {
    VkDeviceSize overallSize = 0;
    
    auto & allModels = this->models.getModels();
    
    for (auto & model : allModels) {
        if (modelsContentType == SSBO) {
            model->setSsboOffset(overallSize);
        }

        for (Mesh & mesh : model->getMeshes()) {            
            VkDeviceSize dataSize = 0;
            switch(modelsContentType) {
                case INDEX:
                    dataSize = mesh.getIndices().size() * sizeof(uint32_t);
                    if (overallSize + dataSize <= maxSize) {
                        memcpy(static_cast<char *>(data) + overallSize, mesh.getIndices().data(), dataSize);
                        overallSize += dataSize;
                    }
                    break;
                case VERTEX:
                    dataSize = mesh.getVertices().size() * sizeof(class Vertex);
                    if (overallSize + dataSize <= maxSize) {
                        memcpy(static_cast<char *>(data)+overallSize, mesh.getVertices().data(), dataSize);
                        overallSize += dataSize;
                    }
                    break;
                case SSBO:
                    TextureInformation textureInfo = mesh.getTextureInformation();
                    MaterialInformation materialInfo = mesh.getMaterialInformation();
                    MeshProperties modelProps = { 
                        textureInfo.ambientTexture,
                        textureInfo.diffuseTexture,
                        textureInfo.specularTexture,
                        textureInfo.normalTexture,
                        materialInfo.ambientColor,
                        materialInfo.emissiveFactor,
                        materialInfo.diffuseColor,
                        materialInfo.opacity,
                        materialInfo.specularColor,
                        materialInfo.shininess
                    };
                    
                    dataSize = sizeof(struct MeshProperties);             
                    if (overallSize + dataSize <= maxSize) {
                        memcpy(static_cast<char *>(data)+overallSize, &modelProps, dataSize);
                        overallSize += dataSize;
                    }
                    break;
            }
        }
    }
}

VkImageView Graphics::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t layerCount) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = layerCount > 1 ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = layerCount;

    VkImageView imageView;
    VkResult ret = vkCreateImageView(this->device, &viewInfo, nullptr, &imageView);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Image View!" << std::endl;
        return nullptr;
    }

    return imageView;
}

void Graphics::cleanupSwapChain() {
    if (this->device == nullptr) return;
    
    vkDeviceWaitIdle(this->device);
    
    for (uint16_t j=0;j<this->depthImages.size();j++) {
        if (this->depthImagesView[j] != nullptr) {
            vkDestroyImageView(this->device, this->depthImagesView[j], nullptr);
            this->depthImagesView[j] = nullptr;            
        }
        if (this->depthImages[j] != nullptr) {
            vkDestroyImage(this->device, this->depthImages[j], nullptr);
            this->depthImages[j] = nullptr;            
        }
        if (this->depthImagesMemory[j] != nullptr) {
            vkFreeMemory(this->device, this->depthImagesMemory[j], nullptr);
            this->depthImagesMemory[j] = nullptr;            
        }
    }

    for (auto & framebuffer : this->swapChainFramebuffers) {
        if (framebuffer != nullptr) {
            vkDestroyFramebuffer(this->device, framebuffer, nullptr);
            framebuffer = nullptr;
        }
    }

    if (this->commandPool != nullptr) {
        vkResetCommandPool(this->device, this->commandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);
    }

    if (this->graphicsPipeline != nullptr) {
        vkDestroyPipeline(this->device, this->graphicsPipeline, nullptr);
        this->graphicsPipeline = nullptr;
    }
    if (this->graphicsPipelineLayout != nullptr) {
        vkDestroyPipelineLayout(this->device, this->graphicsPipelineLayout, nullptr);
        this->graphicsPipelineLayout = nullptr;
    }
    if (this->renderPass != nullptr) {
        vkDestroyRenderPass(this->device, this->renderPass, nullptr);
        this->renderPass = nullptr;
    }

    if (this->skyboxGraphicsPipeline != nullptr) {
        vkDestroyPipeline(this->device, this->skyboxGraphicsPipeline, nullptr);
        this->skyboxGraphicsPipeline = nullptr;
    }
    if (this->skyboxGraphicsPipelineLayout != nullptr) {
        vkDestroyPipelineLayout(this->device, this->skyboxGraphicsPipelineLayout, nullptr);
        this->skyboxGraphicsPipelineLayout = nullptr;
    }
    
    for (auto & imageView : this->swapChainImageViews) {
        if (imageView == nullptr) {
            vkDestroyImageView(this->device, imageView, nullptr);
            imageView = nullptr;
        }
    }

    if (this->swapChain != nullptr) {
        vkDestroySwapchainKHR(this->device, this->swapChain, nullptr);
        this->swapChain = nullptr;
    }
}

void Graphics::cleanupVulkan() {
    if (this->fragShaderModule != nullptr) vkDestroyShaderModule(this->device, this->fragShaderModule, nullptr);
    if (this->vertShaderModule != nullptr) vkDestroyShaderModule(this->device, this->vertShaderModule, nullptr);
    if (this->skyboxFragShaderModule != nullptr) vkDestroyShaderModule(this->device, this->skyboxFragShaderModule, nullptr);
    if (this->skyboxVertShaderModule != nullptr) vkDestroyShaderModule(this->device, this->skyboxVertShaderModule, nullptr);

    if (this->textureSampler != nullptr) {
        vkDestroySampler(this->device, this->textureSampler, nullptr);
    }

    if (this->skyboxSampler != nullptr) {
        vkDestroySampler(this->device, this->skyboxSampler, nullptr);
    }

    if (this->skyboxCubeImage != nullptr) {
        vkDestroyImage(device, this->skyboxCubeImage, nullptr);        
    }

    if (this->skyboxCubeImageMemory != nullptr) {
        vkFreeMemory(device, this->skyboxCubeImageMemory, nullptr);
    }

    if (this->skyboxImageView != nullptr) {
        vkDestroyImageView(device, this->skyboxImageView, nullptr);
    }

    this->models.cleanUpTextures(this->device);
    
    if (this->descriptorPool != nullptr) {
        vkDestroyDescriptorPool(this->device, this->descriptorPool, nullptr);
    }
    if (this->skyboxDescriptorPool != nullptr) {
        vkDestroyDescriptorPool(this->device, this->skyboxDescriptorPool, nullptr);
    }
    
    if (this->descriptorSetLayout != nullptr) {
        vkDestroyDescriptorSetLayout(this->device, this->descriptorSetLayout, nullptr);
    }
    if (this->skyboxDescriptorSetLayout != nullptr) {
        vkDestroyDescriptorSetLayout(this->device, this->skyboxDescriptorSetLayout, nullptr);
    }

    if (this->vertexBuffer != nullptr) vkDestroyBuffer(this->device, this->vertexBuffer, nullptr);
    if (this->vertexBufferMemory != nullptr) vkFreeMemory(this->device, this->vertexBufferMemory, nullptr);

    if (this->indexBuffer != nullptr) vkDestroyBuffer(this->device, this->indexBuffer, nullptr);
    if (this->indexBufferMemory != nullptr) vkFreeMemory(this->device, this->indexBufferMemory, nullptr);

    if (this->ssboBuffer != nullptr) vkDestroyBuffer(this->device, this->ssboBuffer, nullptr);
    if (this->ssboBufferMemory != nullptr) vkFreeMemory(this->device, this->ssboBufferMemory, nullptr);
    
    for (size_t i = 0; i < this->uniformBuffers.size(); i++) {
        if (this->uniformBuffers[i] != nullptr) vkDestroyBuffer(this->device, this->uniformBuffers[i], nullptr);
    }
    for (size_t i = 0; i < this->uniformBuffersMemory.size(); i++) {
        if (this->uniformBuffersMemory[i] != nullptr) vkFreeMemory(this->device, this->uniformBuffersMemory[i], nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (this->renderFinishedSemaphores.size() == MAX_FRAMES_IN_FLIGHT) {
            if (this->renderFinishedSemaphores[i] != nullptr) {
                vkDestroySemaphore(this->device, this->renderFinishedSemaphores[i], nullptr);
            }
            this->renderFinishedSemaphores.clear();
        }

        if (this->imageAvailableSemaphores.size() == MAX_FRAMES_IN_FLIGHT) {
            if (this->imageAvailableSemaphores[i] != nullptr) {
                vkDestroySemaphore(this->device, this->imageAvailableSemaphores[i], nullptr);
            }
            this->imageAvailableSemaphores.clear();
        }
        
        if (this->imagesInFlight.size() == MAX_FRAMES_IN_FLIGHT) {
            this->imagesInFlight.clear();
        }

        if (this->inFlightFences.size() == MAX_FRAMES_IN_FLIGHT) {
            if (this->inFlightFences[i] != nullptr) {
                vkDestroyFence(this->device, this->inFlightFences[i], nullptr);
            }
            this->inFlightFences.clear();
        }
    }
    
    if (this->device != nullptr && this->commandPool != nullptr) {
        vkDestroyCommandPool(this->device, this->commandPool, nullptr);
    }

    if (this->device != nullptr) vkDestroyDevice(this->device, nullptr);

    if (this->vkSurface != nullptr) vkDestroySurfaceKHR(this->vkInstance, this->vkSurface, nullptr);

    if (this->vkInstance != nullptr) vkDestroyInstance(this->vkInstance, nullptr);

    if (this->sdlWindow != nullptr) SDL_DestroyWindow(this->sdlWindow);    
}


