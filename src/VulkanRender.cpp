#include "includes/graphics.h"

Graphics::Graphics() { }

bool Graphics::initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "Could not initialize SDL! Error: " << SDL_GetError() << std::endl;
        return false;
    }

    this->sdlWindow =
            SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (this->sdlWindow == nullptr) {
        std::cerr << "SDL Window could not be created! Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

void Graphics::init(const std::string & appName, uint32_t version, const std::filesystem::path & dir) {
    this->dir = dir;
    if (!this->initSDL()) return;

    SDL_SetWindowTitle(this->sdlWindow, appName.c_str());
    SDL_SetRelativeMouseMode(SDL_FALSE);

    if (this->initVulkan(appName, version)) {
        this->active = true;
        
        TTF_Init();
    }    
}

bool Graphics::prepareModels() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    if (!this->createBuffersFromModel()) return false;
    
    this->prepareModelTextures();
    if (!this->createUniformBuffers()) return false;
    
    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "prepareModels: " << time_span.count() <<  std::endl;

    return true;
}

Graphics & Graphics::instance() {
    static Graphics graphics;
    return graphics;
}


bool Graphics::isActive() {
    return this->active;
}

bool Graphics::createCommandBuffers() {
    this->commandBuffers.resize(this->swapChainFramebuffers.size());
    
    this->startCommandBufferQueue();

    return true;
}

void Graphics::destroyCommandBuffer(VkCommandBuffer commandBuffer) {
    vkFreeCommandBuffers(this->device, this->commandPool, 1, &commandBuffer);
}

VkCommandBuffer Graphics::createCommandBuffer(uint16_t commandBufferIndex) {
    if (this->requiresUpdateSwapChain) return nullptr;
    
    VkCommandBuffer commandBuffer = nullptr;
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = this->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkResult ret = vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Allocate Command Buffer!" << std::endl;
        return nullptr;
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    ret = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to begin Recording Command Buffer!" << std::endl;
        return nullptr;
    }

    if (this->requiresUpdateSwapChain) return nullptr;
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = this->renderPass;
    renderPassInfo.framebuffer = this->swapChainFramebuffers[commandBufferIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = this->swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    
    if (this->requiresUpdateSwapChain) return nullptr;
    
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
    if (this->hasSkybox) {
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            this->skyboxGraphicsPipelineLayout, 0, 1, &this->skyboxDescriptorSets[commandBufferIndex], 0, nullptr);
    
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->skyboxGraphicsPipeline);

        VkDeviceSize offsets[] = {0};
        VkBuffer vertexBuffers[] = {this->skyBoxVertexBuffer};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        
        vkCmdDraw(commandBuffer, SKYBOX_VERTICES.size(), 1, 0, 0);
    }

    if (this->hasTerrain) {
        vkCmdBindDescriptorSets(
            commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            this->terrainGraphicsPipelineLayout, 0, 1, &this->terrainDescriptorSets[commandBufferIndex], 0, nullptr);
    
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->terrainGraphicsPipeline);

        VkDeviceSize offsets[] = {0};
        VkBuffer vertexBuffers[] = {this->terrainVertexBuffer};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        
        vkCmdDraw(commandBuffer, this->terrainVertices.size(), 1, 0, 0);
    }

    if (this->graphicsPipeline != nullptr && !this->requiresUpdateSwapChain) {
        if (this->vertexBuffer != nullptr) {
            vkCmdBindDescriptorSets(
                commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                this->graphicsPipelineLayout, 0, 1, &this->descriptorSets[commandBufferIndex], 0, nullptr);
            
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);

            VkBuffer vertexBuffers[] = {this->vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        }
            
        if (this->indexBuffer != nullptr) {
            vkCmdBindIndexBuffer(commandBuffer, this->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            this->draw(commandBuffer, true);
        } else {
            this->draw(commandBuffer, false);       
        }
    }

    vkCmdEndRenderPass(commandBuffer);

    ret = vkEndCommandBuffer(commandBuffer);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to end  Recording Command Buffer!" << std::endl;
        return nullptr;
    }

    return commandBuffer;
}

void Graphics::updateUniformBuffer(uint32_t currentImage) {
    ModelUniforms modelUniforms {};
    modelUniforms.camera = glm::vec4(Camera::instance()->getPosition(),1);
    modelUniforms.sun = glm::vec4(0.0f, 100.0f, 100.0f, 1);
    modelUniforms.viewMatrix = Camera::instance()->getViewMatrix();
    modelUniforms.projectionMatrix = Camera::instance()->getProjectionMatrix();

    void* data;
    vkMapMemory(this->device, this->uniformBuffersMemory[currentImage], 0, sizeof(modelUniforms), 0, &data);
    memcpy(data, &modelUniforms, sizeof(modelUniforms));
    vkUnmapMemory(this->device, this->uniformBuffersMemory[currentImage]);
}
    
void Graphics::drawFrame() {    
    std::chrono::high_resolution_clock::time_point frameStart = std::chrono::high_resolution_clock::now();
    
    if (this->requiresUpdateSwapChain) {
        this->updateSwapChain();
        return;
    }

    VkResult ret = vkWaitForFences(device, 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
    if (ret != VK_SUCCESS) {
        this->requiresUpdateSwapChain = true;
        return;
    }
    
    uint32_t imageIndex;
    ret = vkAcquireNextImageKHR(
        this->device, this->swapChain, UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Acquire Next Image" << std::endl;
        this->requiresUpdateSwapChain = true;
        return;
    }

    if (this->commandBuffers[imageIndex] != nullptr) {
        this->workerQueue.queueCommandBufferForDeletion(this->commandBuffers[imageIndex]);
    }

    VkCommandBuffer latestCommandBuffer = this->workerQueue.getNextCommandBuffer(imageIndex);
    std::chrono::high_resolution_clock::time_point nextBufferFetchStart = std::chrono::high_resolution_clock::now();
    while (latestCommandBuffer == nullptr) {
        std::chrono::duration<double, std::milli> fetchPeriod = std::chrono::high_resolution_clock::now() - nextBufferFetchStart;
        if (fetchPeriod.count() > 2000) {
            std::cout << "Could not get new buffer for quite a while!" << std::endl;
            break;
        }
        latestCommandBuffer = this->workerQueue.getNextCommandBuffer(imageIndex);
    }
    std::chrono::duration<double, std::milli> timer = std::chrono::high_resolution_clock::now() - nextBufferFetchStart;
    //if (timer.count() < 50) {
    //    std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(50 - timer.count()));
    //}
    //timer = std::chrono::high_resolution_clock::now() - nextBufferFetchStart;
    //std::cout << "Fetch Time: " << timer.count() << " | " << this->workerQueue.getNumberOfItems(imageIndex) << std::endl;
    
    if (latestCommandBuffer == nullptr) return;
    this->commandBuffers[imageIndex] = latestCommandBuffer;

    this->updateUniformBuffer(imageIndex);
        
    if (this->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        ret = vkWaitForFences(device, 1, &this->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        if (ret != VK_SUCCESS) {
             std::cerr << "vkWaitForFences 2 Failed" << std::endl;
        }
    }
    this->imagesInFlight[imageIndex] = this->inFlightFences[this->currentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {this->imageAvailableSemaphores[this->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = this->commandBuffers.empty() ? 0 : 1;
    submitInfo.pCommandBuffers = &this->commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {this->renderFinishedSemaphores[this->currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    ret = vkResetFences(this->device, 1, &this->inFlightFences[this->currentFrame]);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Reset Fence!" << std::endl;
    }

    ret = vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, this->inFlightFences[this->currentFrame]);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Submit Draw Command Buffer!" << std::endl;
    }
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {this->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    ret = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Present Swap Chain Image!" << std::endl;
        return;
    }
    
    this->currentFrame = (this->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    ++this->frameCount;

    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_span = now -frameStart;
    
    this->setDeltaTime(time_span.count());
    
    time_span = now - this->lastTimeMeasure;
    if (time_span.count() >= 1000) {
        this->lastTimeMeasure = now;
        std::cout << "FPS: " << this->frameCount <<  std::endl;
        this->frameCount = 0;
    }
}


ModelSummary Graphics::getModelsBufferSizes() {
    ModelSummary bufferSizes;
    
    auto & allModels = this->models.getModels();
    
    for (auto & model : allModels) {
        auto meshes = model->getMeshes();
        for (Mesh & mesh : meshes) {
            VkDeviceSize vertexSize = mesh.getVertices().size();
            VkDeviceSize indexSize = mesh.getIndices().size();

            bufferSizes.vertexBufferSize += vertexSize * sizeof(class Vertex);
            bufferSizes.indexBufferSize += indexSize * sizeof(uint32_t);
            bufferSizes.ssboBufferSize += sizeof(struct MeshProperties);
        }
    }
    
    std::cout << "Vertex Buffer Size: " << bufferSizes.vertexBufferSize << std::endl;
    std::cout << "Index Buffer Size: " << bufferSizes.indexBufferSize << std::endl;
    std::cout << "SSBO Buffer Size: " << bufferSizes.ssboBufferSize << std::endl;
    
    return bufferSizes;
}

bool Graphics::createBuffersFromModel() {
    ModelSummary bufferSizes = this->getModelsBufferSizes();
     
    if (bufferSizes.vertexBufferSize == 0) return true;

    if (this->vertexBuffer != nullptr) vkDestroyBuffer(this->device, this->vertexBuffer, nullptr);
    if (this->vertexBufferMemory != nullptr) vkFreeMemory(this->device, this->vertexBufferMemory, nullptr);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    if (!this->createBuffer(
            bufferSizes.vertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory)) {
        std::cerr << "Failed to get Create Staging Buffer" << std::endl;
        return false;
    }

    void* data = nullptr;
    vkMapMemory(this->device, stagingBufferMemory, 0, bufferSizes.vertexBufferSize, 0, &data);
    this->copyModelsContentIntoBuffer(data, VERTEX, bufferSizes.vertexBufferSize);
    vkUnmapMemory(this->device, stagingBufferMemory);

    if (!this->createBuffer(
            bufferSizes.vertexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->vertexBuffer, this->vertexBufferMemory)) {
        std::cerr << "Failed to get Create Vertex Buffer" << std::endl;
        return false;
    }

    this->copyBuffer(stagingBuffer,this->vertexBuffer, bufferSizes.vertexBufferSize);

    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);
    
    // meshes (SSBOs)
    if (!this->createSsboBufferFromModel(bufferSizes.ssboBufferSize, false)) return false;
        
    // indices
    if (bufferSizes.indexBufferSize == 0) return true;

    if (this->indexBuffer != nullptr) vkDestroyBuffer(this->device, this->indexBuffer, nullptr);
    if (this->indexBufferMemory != nullptr) vkFreeMemory(this->device, this->indexBufferMemory, nullptr);

    if (!this->createBuffer(
            bufferSizes.indexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory)) {
        std::cerr << "Failed to get Create Staging Buffer" << std::endl;
        return false;
    }

    data = nullptr;
    vkMapMemory(this->device, stagingBufferMemory, 0, bufferSizes.indexBufferSize, 0, &data);
    this->copyModelsContentIntoBuffer(data, INDEX, bufferSizes.indexBufferSize);
    vkUnmapMemory(this->device, stagingBufferMemory);

    if (!this->createBuffer(
            bufferSizes.indexBufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->indexBuffer, this->indexBufferMemory)) {
        std::cerr << "Failed to get Create Vertex Buffer" << std::endl;
        return false;
    }
    
    this->copyBuffer(stagingBuffer,this->indexBuffer, bufferSizes.indexBufferSize);

    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);
    
    return true;
}

bool Graphics::createSsboBufferFromModel(VkDeviceSize bufferSize, bool makeHostWritable)
{
    if (bufferSize == 0) return true;

    if (this->ssboBuffer != nullptr) vkDestroyBuffer(this->device, this->ssboBuffer, nullptr);
    if (this->ssboBufferMemory != nullptr) vkFreeMemory(this->device, this->ssboBufferMemory, nullptr);

    if (!makeHostWritable) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        if (!this->createBuffer(bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory)) {
            std::cerr << "Failed to get Create Staging Buffer" << std::endl;
            return false;
        }

        void* data = nullptr;
        vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data);
        this->copyModelsContentIntoBuffer(data, SSBO, bufferSize);
        vkUnmapMemory(this->device, stagingBufferMemory);

        if (!this->createBuffer(bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                this->ssboBuffer, this->ssboBufferMemory)) {
            std::cerr << "Failed to get Create SSBO Buffer" << std::endl;
            return false;
        }

        this->copyBuffer(stagingBuffer,this->ssboBuffer, bufferSize);

        vkDestroyBuffer(this->device, stagingBuffer, nullptr);
        vkFreeMemory(this->device, stagingBufferMemory, nullptr);        
    } else {
        if (!this->createBuffer(
                bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                this->ssboBuffer, this->ssboBufferMemory)) {
            std::cerr << "Failed to get Create Vertex Buffer" << std::endl;
            return false;
        }

        void * data = nullptr;
        vkMapMemory(this->device, ssboBufferMemory, 0, bufferSize, 0, &data);
        this->copyModelsContentIntoBuffer(data, SSBO, bufferSize);
        vkUnmapMemory(this->device, ssboBufferMemory);        
    }
    
    return true;
}

void Graphics::draw(VkCommandBuffer & commandBuffer, bool useIndices) {
    VkDeviceSize lastVertexOffset = 0;
    VkDeviceSize lastIndexOffset = 0;
    uint32_t firstInstanceMesh = 0;

    auto & allModels = this->models.getModels();
    
    for (auto & model :  allModels) {
        auto meshes = model->getMeshes();
        
        for (Mesh & mesh : meshes) {
            VkDeviceSize vertexSize = mesh.getVertices().size();
            VkDeviceSize indexSize = mesh.getIndices().size();
            
            if (this->requiresUpdateSwapChain) return;
            
            auto allComponents = this->components.getAllComponentsForModel(model->getId());
            for (auto & comp : allComponents) {
                if (!comp->isVisible() || (this->useFrustumCulling && !Camera::instance()->isInFrustum(comp->getPosition()))) continue;
                
                ModelProperties props = { comp->getModelMatrix()};
                
                vkCmdPushConstants(
                    commandBuffer, this->graphicsPipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(struct ModelProperties), &props);

                if (useIndices) {                
                    vkCmdDrawIndexed(commandBuffer, indexSize , 1, lastIndexOffset, lastVertexOffset, firstInstanceMesh);
                } else {
                    vkCmdDraw(commandBuffer, vertexSize, 1, 0, firstInstanceMesh);
                }
            }
                        
            lastIndexOffset += indexSize;
            lastVertexOffset += vertexSize;
            firstInstanceMesh++;
        }
    }
}

Component * Graphics::addComponentWithModel(std::string id, std::string modelId) {
    Model * model = this->models.findModel(modelId);
    if (model == nullptr) return nullptr;
    
    return this->components.addComponent(new Component(id, model));
}


void Graphics::addText(std::string id, std::string font, std::string text, uint16_t size) {
    this->models.addTextModel(id, this->getAppPath(FONTS) / font, text, size);
}

void Graphics::addModel(Model * model) {
     std::unique_ptr<Model> modelPtr(model);
     if (model->hasBeenLoaded()) this->models.addModel(modelPtr.release());
}

void Graphics::addModel(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices, std::string name) {
     std::unique_ptr<Model> model = std::make_unique<Model>(vertices, indices, name);
     if (model->hasBeenLoaded()) this->models.addModel(model.release());
}

void Graphics::addModel(const std::string & dir, const std::string & file) {
     std::unique_ptr<Model> model = std::make_unique<Model>(dir, file);
     if (model->hasBeenLoaded()) this->models.addModel(model.release());
}

void Graphics::toggleWireFrame() {
    this->showWireFrame = !this->showWireFrame;
    this->requiresUpdateSwapChain = true;
}

SDL_Window * Graphics::getSdlWindow() {
    return this->sdlWindow;
}

void Graphics::prepareModelTextures() {
    auto & textures = this->models.getTextures();
    
    // put in one dummy one to satify shader if we have none...
    if (textures.empty()) {
        std::unique_ptr<Texture> emptyTexture = std::make_unique<Texture>(true, this->getWindowExtent());
        emptyTexture->setId(0);
        if (emptyTexture->isValid()) {
            textures["dummy"] = std::move(emptyTexture);
            std::cout << "dummy" << std::endl;
        }
    }

    for (auto & texture : textures) {
        VkDeviceSize imageSize = texture.second->getSize();
        
        VkBuffer stagingBuffer = nullptr;
        VkDeviceMemory stagingBufferMemory = nullptr;
        if (!this->createBuffer(
            imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory)) {
                std::cerr << "Failed to Create Texture Staging Buffer" << std::endl;
                return;
        }

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, texture.second->getPixels(), static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);
        
        VkImage textureImage = nullptr;
        VkDeviceMemory textureImageMemory = nullptr;
        
        if (!this->createImage(
            texture.second->getWidth(), texture.second->getHeight(), 
            texture.second->getImageFormat(), 
            VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            textureImage, textureImageMemory)) {
                std::cerr << "Failed to Create Texture Image" << std::endl;
                return;
        }

        transitionImageLayout(textureImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        this->copyBufferToImage(
            stagingBuffer, textureImage, static_cast<uint32_t>(texture.second->getWidth()), static_cast<uint32_t>(texture.second->getHeight()));
        transitionImageLayout(
            textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(this->device, stagingBuffer, nullptr);
        vkFreeMemory(this->device, stagingBufferMemory, nullptr);
        
        if (textureImage != nullptr) texture.second->setTextureImage(textureImage);
        if (textureImageMemory != nullptr) texture.second->setTextureImageMemory(textureImageMemory);
        
        
        VkImageView textureImageView = this->createImageView(textureImage, texture.second->getImageFormat(), VK_IMAGE_ASPECT_COLOR_BIT);
        if (textureImageView != nullptr) texture.second->setTextureImageView(textureImageView);
        
        texture.second->freeSurface();
    }
    
    std::cout << "Number of Textures: " << textures.size() << std::endl;
}

bool Graphics::updateSwapChain() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    if (this->device == nullptr) return false;

    this->stopCommandBufferQueue();

    this->cleanupSwapChain();
    
    this->requiresUpdateSwapChain = false;

    if (!this->createSwapChain()) return false;
    if (!this->createImageViews()) return false;
    if (!this->createRenderPass()) return false;

    if (this->hasTerrain) {
        if (!this->createTerrainGraphicsPipeline()) return false;
    }
    
    if (this->hasSkybox) {
        if (!this->createSkyboxGraphicsPipeline()) return false;
    }
    
    if (!this->createGraphicsPipeline()) return false;
    if (!this->createDepthResources()) return false;
    if (!this->createFramebuffers()) return false;

    Camera::instance()->setAspectRatio(static_cast<float>(this->swapChainExtent.width) / this->swapChainExtent.height);
    
    if (!this->createCommandBuffers()) return false;
        
    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "updateSwapChain: " << time_span.count() <<  std::endl;

    return true;
}

Models & Graphics::getModels() {
    return this->models;
}

Components & Graphics::getComponents() {
    return this->components;
}

void Graphics::prepareComponents() {
    this->components.initWithModelIds(this->models.getModelIds());
}

double Graphics::getDeltaTime() {
    return this->deltaTime;
}

void Graphics::setDeltaTime(double deltaTime) {
    this->deltaTime = deltaTime;
}

void Graphics::setLastTimeMeasure(std::chrono::high_resolution_clock::time_point time) {
    this->lastTimeMeasure = time;
}

void Graphics::startCommandBufferQueue() {
    if (!this->isActive() || this->workerQueue.isRunning()) return;
    
    this->workerQueue.startQueue(
        std::bind(&Graphics::createCommandBuffer, this, std::placeholders::_1),
        std::bind(&Graphics::destroyCommandBuffer , this, std::placeholders::_1), 
        this->swapChainFramebuffers.size());
}

void Graphics::stopCommandBufferQueue() {
    this->workerQueue.stopQueue();
}

std::filesystem::path Graphics::getAppPath(APP_PATHS appPath) {
    switch(appPath) {
        case SHADERS:
            return this->dir / "res/shaders";
        case MODELS:
            return this->dir / "res/models";
        case FONTS:
            return this->dir / "res/fonts";
        case ROOT:
        default:
            return this->dir;
    }
}

Graphics::~Graphics() {
    this->stopCommandBufferQueue();

    this->cleanupSwapChain();
    this->cleanupVulkan();

    TTF_Quit();
    
    SDL_Quit();
}
