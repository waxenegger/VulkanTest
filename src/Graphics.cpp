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

void Graphics::init(const std::string & appName, uint32_t version, const std::string & dir) {
    this->dir = dir;
    if (!this->initSDL()) return;

    SDL_SetWindowTitle(this->sdlWindow, appName.c_str());
    SDL_SetRelativeMouseMode(SDL_FALSE);

    if (this->initVulkan(appName, version)) {
        this->active = true;
    }
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
    
    this->hasSkybox = this->createSkybox();
        
    if (!this->createShaderStageInfo()) return false;
    if (this->hasSkybox) {
        if (!this->createSkyboxShaderStageInfo()) return false;        
    }

    if (this->hasSkybox) {
        if (!this->createSkyboxDescriptorPool()) return false;
    }
    if (!this->createDescriptorPool()) return false;
    
    if (!this->createSyncObjects()) return false;
    
    return true;
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

bool Graphics::getSurfaceCapabilities(VkSurfaceCapabilitiesKHR & surfaceCapabilities) {
    if (this->sdlWindow == nullptr || this->vkSurface == nullptr || this->physicalDevice == nullptr) return false;

    const VkResult ret = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(this->physicalDevice, this->vkSurface, &surfaceCapabilities);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to get Device Surface Capabilites" << std::endl;
        return false;
    }

    return true;
}

bool Graphics::getSwapChainExtent(VkSurfaceCapabilitiesKHR & surfaceCapabilities) {
    if (this->sdlWindow == nullptr || this->vkSurface == nullptr) return false;

    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        this->swapChainExtent.width = surfaceCapabilities.currentExtent.width;
        this->swapChainExtent.height = surfaceCapabilities.currentExtent.height;

        return true;
    } else {
        int width = 0;
        int height = 0;
        SDL_Vulkan_GetDrawableSize(this->sdlWindow, &width, &height);
        
        this->swapChainExtent.width = std::max(
            surfaceCapabilities.minImageExtent.width,
            std::min(surfaceCapabilities.maxImageExtent.width,
            static_cast<uint32_t>(width)));
        this->swapChainExtent.height = std::max(
            surfaceCapabilities.minImageExtent.height,
            std::min(surfaceCapabilities.maxImageExtent.height,
            static_cast<uint32_t>(height)));
        
        return true;
    }
}



VkExtent2D Graphics::getWindowExtent() {
    return this->swapChainExtent;
}

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

    uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
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

bool Graphics::createSkybox() {
    std::array<std::string, 6> skyboxCubeImageLocations = {
        "sky_right.png", "sky_left.png", "sky_top.png", "sky_bottom.png", "sky_front.png", "sky_back.png" 
    };
    std::vector<std::unique_ptr<Texture>> skyboxCubeTextures;

    for (auto & s : skyboxCubeImageLocations) {
        std::unique_ptr<Texture> texture = std::make_unique<Texture>();
        texture->setPath(this->dir + "res/models/" + s);
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

std::tuple<VkPhysicalDevice, int> Graphics::pickBestPhysicalDeviceAndQueueIndex() {
    std::tuple<VkPhysicalDevice, int> choice = std::make_tuple(nullptr, -1);

    if (this->vkPhysicalDevices.empty()) return choice;

    int highestScore = 0;
    int i=0;
    for (auto & device : this->vkPhysicalDevices) {
        const std::tuple<int, int> scoreAndQueueIndex = this->ratePhysicalDevice(device);
        if (std::get<0>(scoreAndQueueIndex) > highestScore && std::get<1>(scoreAndQueueIndex) != -1) {
            highestScore = std::get<0>(scoreAndQueueIndex);
            choice = std::make_tuple(device, std::get<1>(scoreAndQueueIndex));
        }
        i++;
    }

    return choice;
}

void Graphics::listPhysicalDeviceExtensions() {
    if (this->physicalDevice == nullptr) return;
    
    std::vector<VkExtensionProperties> availableExtensions = this->queryPhysicalDeviceExtensions(this->physicalDevice);

    std::cout << "Device Extensions" << std::endl;
    for (auto & extProp : availableExtensions) {
          std::cout << "\t" << extProp.extensionName << std::endl;
    }
}


bool Graphics::doesPhysicalDeviceSupportExtension(const VkPhysicalDevice & device, const std::string extension) {
    std::vector<VkExtensionProperties> availableExtensions = this->queryPhysicalDeviceExtensions(device);

    for (auto & extProp : availableExtensions) {
        const std::string extName = Utils::convertCcharToString(extProp.extensionName);
        if (extName.compare(extension) == 0) {
            return true;
        }
    }

    return false;
}


std::vector<VkSurfaceFormatKHR> Graphics::queryPhysicalDeviceSurfaceFormats(const VkPhysicalDevice & device) {
    uint32_t formatCount = 0;
    std::vector<VkSurfaceFormatKHR> formats;

    if (this->vkSurface == nullptr) return formats;

    VkResult ret = vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->vkSurface, &formatCount, nullptr);
    ASSERT_VULKAN(ret);

    if (ret == VK_SUCCESS && formatCount > 0) {
        formats.resize(formatCount);
        ret = vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->vkSurface, &formatCount, formats.data());
        ASSERT_VULKAN(ret);
    }

    return formats;
}

void Graphics::listPhysicalDeviceSurfaceFormats(const VkPhysicalDevice & device) {
    if (this->vkSurface == nullptr) return;

    std::vector<VkSurfaceFormatKHR> availableSurfaceFormats = this->queryPhysicalDeviceSurfaceFormats(device);

    std::cout << "Surface Formats" << std::endl;
    for (auto & surfaceFormat : availableSurfaceFormats) {
          std::cout << "\t" << surfaceFormat.format << std::endl;
    }

}

bool Graphics::isPhysicalDeviceSurfaceFormatsSupported(const VkPhysicalDevice & device, const VkSurfaceFormatKHR & format) {
    if (this->vkSurface == nullptr) return false;

    std::vector<VkSurfaceFormatKHR> availableSurfaceFormats = this->queryPhysicalDeviceSurfaceFormats(device);

    for (auto & surfaceFormat : availableSurfaceFormats) {
        if (format.format == surfaceFormat.format && format.colorSpace == surfaceFormat.colorSpace) {
            return true;
        }
    }

    return false;

}

std::vector<VkPresentModeKHR> Graphics::queryDeviceSwapModes() {
    uint32_t swapModesCount = 0;
    std::vector<VkPresentModeKHR> swapModes;

    if (this->physicalDevice == nullptr || this->vkSurface == nullptr) return swapModes;

    VkResult ret = vkGetPhysicalDeviceSurfacePresentModesKHR(this->physicalDevice, this->vkSurface, &swapModesCount, nullptr);
    ASSERT_VULKAN(ret);

    if (ret == VK_SUCCESS && swapModesCount > 0) {
        swapModes.resize(swapModesCount);
        ret = vkGetPhysicalDeviceSurfacePresentModesKHR(this->physicalDevice, this->vkSurface, &swapModesCount, swapModes.data());
        ASSERT_VULKAN(ret);
    }

    return swapModes;
}

VkPresentModeKHR Graphics::pickBestDeviceSwapMode(const std::vector<VkPresentModeKHR> & availableSwapModes) {
    for (auto & swapMode : availableSwapModes) {
        if (swapMode == VK_PRESENT_MODE_MAILBOX_KHR) return swapMode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}


std::tuple<int,int> Graphics::ratePhysicalDevice(const VkPhysicalDevice & device) {
    // check if physical device supports swap chains and required surface format
    if (!this->doesPhysicalDeviceSupportExtension(device, "VK_KHR_swapchain") ||
            !this->isPhysicalDeviceSurfaceFormatsSupported(device, this->swapChainImageFormat)) {
        return std::make_tuple(0, -1);
    };

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    switch(deviceProperties.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
            score = 1500;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score = 1000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score = 500;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score = 250;
            break;
        default:
            break;
    }

    if (deviceFeatures.geometryShader) {
        score += 5;
    }

    const std::vector<VkQueueFamilyProperties> queueFamiliesProperties = this->getPhysicalDeviceQueueFamilyProperties(device);

    int queueChoice = -1;
    int lastBestQueueScore = 0;
    bool supportsGraphicsAndPresentationQueue = false;

    int j=0;
    for (auto & queueFamilyProperties : queueFamiliesProperties) {
        int queueScore = 0;

        if ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            VkBool32 supportsPresentation = false;
            if (this->vkSurface != nullptr) vkGetPhysicalDeviceSurfaceSupportKHR(device, j, this->vkSurface, &supportsPresentation);
            if (supportsPresentation) supportsGraphicsAndPresentationQueue = true;

            queueScore += 10 * queueFamilyProperties.queueCount;

            if (queueScore > lastBestQueueScore) {
                lastBestQueueScore = queueScore;
                queueChoice = j;
            }
        }

        j++;
    }

    // we need a device with a graphics queue, alright
    if (!supportsGraphicsAndPresentationQueue) return std::make_tuple(0, -1);

    return std::make_tuple(score+lastBestQueueScore, queueChoice);
}


void Graphics::queryVkInstanceExtensions() {
    uint32_t extensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(this->sdlWindow, &extensionCount, nullptr) == SDL_FALSE) {
        std::cerr << "Could not get SDL Vulkan Extensions: " << SDL_GetError() << std::endl;
    } else {
        this->vkExtensionNames.resize(extensionCount);
        SDL_Vulkan_GetInstanceExtensions(this->sdlWindow, &extensionCount, this->vkExtensionNames.data());
    }
}

std::vector<VkExtensionProperties> Graphics::queryPhysicalDeviceExtensions(const VkPhysicalDevice & device) {
    uint32_t deviceExtensionCount = 0;

    std::vector<VkExtensionProperties> extensions;

    VkResult ret = vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);
    ASSERT_VULKAN(ret);

    if (ret == VK_SUCCESS && deviceExtensionCount > 0) {
        extensions.resize(deviceExtensionCount);
        ret = vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, extensions.data());
        ASSERT_VULKAN(ret);
    }

    return extensions;
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

void Graphics::queryPhysicalDevices() {
    uint32_t physicalDeviceCount = 0;
    VkResult ret;

    ret = vkEnumeratePhysicalDevices(this->vkInstance, &physicalDeviceCount, nullptr);
    ASSERT_VULKAN(ret);

    if (ret == VK_SUCCESS) {
        this->vkPhysicalDevices.resize(physicalDeviceCount);
        ret = vkEnumeratePhysicalDevices(this->vkInstance, &physicalDeviceCount, this->vkPhysicalDevices.data());
        ASSERT_VULKAN(ret);
    }
}

void Graphics::listVkPhysicalDevices() {
    if (this->vkPhysicalDevices.empty()) return;

    VkPhysicalDeviceProperties physicalProperties;

    std::cout << "Physical Devices:" << std::endl;
    for (auto & device : this->vkPhysicalDevices) {
        vkGetPhysicalDeviceProperties(device, &physicalProperties);
        std::cout << "\t" << physicalProperties.deviceName << "\t[Type: " <<
                physicalProperties.deviceType << "]" << std::endl;
    }
}

void Graphics::listVkExtensions() {
    if (this->vkExtensionNames.empty()) return;

    std::cout << "Extensions: " << std::endl;
    for (auto & extension : this->vkExtensionNames) {
        std::cout << "\t" << extension << std::endl;
    }
}

void Graphics::listVkLayerNames() {
    uint32_t layerCount = 0;
    VkResult ret;

    ret = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    ASSERT_VULKAN(ret);

    std::vector<VkLayerProperties> availableLayers(layerCount);

    if (ret == VK_SUCCESS) {
        ret = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        ASSERT_VULKAN(ret);

        if (availableLayers.empty()) return;

        std::cout << "Layers: " << std::endl;
        for (auto & layer : availableLayers) {
           std::cout << "\t" << layer.layerName << std::endl;
        }
    }
}

const std::vector<VkQueueFamilyProperties> Graphics::getPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device) {
    uint32_t numberOfDeviceQueuerFamilyProperties = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfDeviceQueuerFamilyProperties, nullptr);

    std::vector<VkQueueFamilyProperties> vkQueueFamilyProperties(numberOfDeviceQueuerFamilyProperties);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfDeviceQueuerFamilyProperties, vkQueueFamilyProperties.data());

    return vkQueueFamilyProperties;
}

void Graphics::listVkPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device) {
    const std::vector<VkQueueFamilyProperties> queueFamiliesProperties = this->getPhysicalDeviceQueueFamilyProperties(device);
    int c = 0;
    for (auto & queueFamilyProperties : queueFamiliesProperties) {
        std::cout << "Queue Index: " << c << std::endl;
        std::cout << "\tQueue Count: " << queueFamilyProperties.queueCount << std::endl;
        std::cout << "\tVK_QUEUE_GRAPHICS_BIT: " << ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) << std::endl;
        std::cout << "\tVK_QUEUE_TRANSFER_BIT: " << ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) << std::endl;
        std::cout << "\tVK_QUEUE_COMPUTE_BIT: " << ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) << std::endl;
        std::cout << "\tVK_QUEUE_SPARSE_BINDING_BIT: " << ((queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) << std::endl;
        c++;
    }
}

Graphics & Graphics::instance() {
    static Graphics graphics;
    return graphics;
}


bool Graphics::isActive() {
    return this->active;
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
    if (!Utils::readFile(this->dir + "/res/shaders/vert.spv", vertShaderCode) ||
            !Utils::readFile(this->dir + "/res/shaders/frag.spv", fragShaderCode)) {
        std::cerr << "Failed to read shader files: " << (this->dir + "/res/shaders/") << std::endl;
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

bool Graphics::createSkyboxShaderStageInfo() {
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    if (!Utils::readFile(this->dir + "/res/shaders/skybox_vert.spv", vertShaderCode) ||
            !Utils::readFile(this->dir + "/res/shaders/skybox_frag.spv", fragShaderCode)) {
        std::cerr << "Failed to read shader files: " << (this->dir + "/res/shaders/") << std::endl;
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
        const std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions = Vertex::getAttributeDescriptions();

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

bool Graphics::createGraphicsPipeline() {
    if (this->vertexBuffer == nullptr) return true;
    
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    const VkVertexInputBindingDescription bindingDescription = Vertex::getBindingDescription();
    const std::array<VkVertexInputAttributeDescription, 6> attributeDescriptions = Vertex::getAttributeDescriptions();

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

    VkResult ret = vkCreatePipelineLayout(this->device, &pipelineLayoutInfo, nullptr, &this->context.graphicsPipelineLayout);
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
    pipelineInfo.layout = this->context.graphicsPipelineLayout;
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

bool Graphics::createCommandBuffers() {
    this->context.commandBuffers.resize(this->swapChainFramebuffers.size());

    for (uint16_t i=0; i<this->context.commandBuffers.size();i++) {
        if (!this->createCommandBuffer(i)) return false;
    }

    return true;
}

bool Graphics::createCommandBuffer(uint16_t commandBufferIndex) {
    VkResult ret;
    if (this->context.commandBuffers[commandBufferIndex] == nullptr) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = this->commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        ret = vkAllocateCommandBuffers(device, &allocInfo, &this->context.commandBuffers[commandBufferIndex]);
        if (ret != VK_SUCCESS) {
            std::cerr << "Failed to Allocate Command Buffer!" << std::endl;
            return false;
        }
    }

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    ret = vkBeginCommandBuffer(this->context.commandBuffers[commandBufferIndex], &beginInfo);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to begin Recording Command Buffer!" << std::endl;
        return false;
    }

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
    
    vkCmdBeginRenderPass(this->context.commandBuffers[commandBufferIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    if (this->hasSkybox) {
        vkCmdBindDescriptorSets(
            this->context.commandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, 
            this->skyboxGraphicsPipelineLayout, 0, 1, &this->skyboxDescriptorSets[commandBufferIndex], 0, nullptr);
    
        vkCmdBindPipeline(this->context.commandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, this->skyboxGraphicsPipeline);

        this->drawSkybox(this->context, commandBufferIndex);
    }
    
    if (this->graphicsPipeline != nullptr) {
        if (this->vertexBuffer != nullptr) {
            vkCmdBindDescriptorSets(
                this->context.commandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, 
                this->context.graphicsPipelineLayout, 0, 1, &this->descriptorSets[commandBufferIndex], 0, nullptr);
            
            vkCmdBindPipeline(this->context.commandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);

            VkBuffer vertexBuffers[] = {this->vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(this->context.commandBuffers[commandBufferIndex], 0, 1, vertexBuffers, offsets);
        }
            
        if (this->indexBuffer != nullptr) {
            vkCmdBindIndexBuffer(this->context.commandBuffers[commandBufferIndex], this->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            this->draw(this->context, commandBufferIndex, true);
        } else {
            this->draw(this->context, commandBufferIndex, false);                
        }
    }

    vkCmdEndRenderPass(this->context.commandBuffers[commandBufferIndex]);

    ret = vkEndCommandBuffer(this->context.commandBuffers[commandBufferIndex]);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to end  Recording Command Buffer!" << std::endl;
        return false;
    }

    return true;
}

bool Graphics::updateSwapChain() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    if (this->device == nullptr) return false;

    this->cleanupSwapChain();

    if (!this->createSwapChain()) return false;
    if (!this->createImageViews()) return false;
    if (!this->createRenderPass()) return false;
    
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

    if (this->commandPool != nullptr && !this->context.commandBuffers.empty()) {
        for (auto & commandBuffer : this->context.commandBuffers) {
            if (commandBuffer != nullptr) {
                vkFreeCommandBuffers(this->device, this->commandPool, 1, &commandBuffer);   
                commandBuffer = nullptr;
            }
        }
    }

    if (this->graphicsPipeline != nullptr) {
        vkDestroyPipeline(this->device, this->graphicsPipeline, nullptr);
        this->graphicsPipeline = nullptr;
    }
    if (this->context.graphicsPipelineLayout != nullptr) {
        vkDestroyPipelineLayout(this->device, this->context.graphicsPipelineLayout, nullptr);
        this->context.graphicsPipelineLayout = nullptr;
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

bool Graphics::createCommandPool() {
    if (this->device == nullptr) return false;
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = this->graphicsQueueIndex;

    VkResult ret = vkCreateCommandPool(this->device, &poolInfo, nullptr, &this->commandPool);
    ASSERT_VULKAN(ret);

    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Command Pool" << std::endl;
        return false;
    }

    return true;
}

void Graphics::updateUniformBuffer(uint32_t currentImage) {
    ModelUniforms modelUniforms {};
    modelUniforms.camera = glm::vec4(Camera::instance()->getPosition(),1);
    modelUniforms.sun = glm::vec4(0.0f, 0.0f, 50.0f, 1);
    modelUniforms.viewMatrix = Camera::instance()->getViewMatrix();
    modelUniforms.projectionMatrix = Camera::instance()->getProjectionMatrix();

    void* data;
    vkMapMemory(this->device, this->uniformBuffersMemory[currentImage], 0, sizeof(modelUniforms), 0, &data);
    memcpy(data, &modelUniforms, sizeof(modelUniforms));
    vkUnmapMemory(this->device, this->uniformBuffersMemory[currentImage]);
}

void Graphics::updateSsboBuffer() {
    auto dirtyComponents = this->components.getSsboComponentsThatNeedUpdate();
    
    for (auto & c : dirtyComponents) {
        void* data;
        auto model = c->getModel();
        if (model != nullptr) {
            auto numberOfCompsForModel = this->components.getAllComponentsForModel(model->getPath(), true).size();
            auto meshes = model->getMeshes();
            uint32_t m=0;
            for (auto & mesh : meshes) {
                vkMapMemory(this->device, 
                            this->ssboBufferMemory, model->getSsboOffset() + numberOfCompsForModel * m * sizeof(struct MeshProperties) +
                            c->getSsboIndex() * sizeof(struct MeshProperties), 
                            sizeof(struct MeshProperties), 0, &data);
                TextureInformation textureInfo = mesh.getTextureInformation();
                MeshProperties props = {
                    textureInfo.ambientTexture,
                    textureInfo.diffuseTexture,
                    textureInfo.specularTexture,
                    textureInfo.normalTexture
                };
                memcpy(data, &props, sizeof(struct MeshProperties));
                vkUnmapMemory(this->device, this->ssboBufferMemory);
                m++;
            }
        }
        c->markSsboAsNotDirty();
    }
}

void Graphics::updateScene(uint16_t commandBufferIndex, bool waitForFences) {
    VkResult ret;
    if (waitForFences && this->imagesInFlight[commandBufferIndex] != VK_NULL_HANDLE) {
        ret = vkWaitForFences(device, 1, &this->inFlightFences[commandBufferIndex], VK_TRUE, UINT64_MAX);
        if (ret != VK_SUCCESS) {
            std::cerr << "vkWaitForFences Failed" << std::endl;
        }
    }
    
    this->createCommandBuffer(commandBufferIndex);
    
    if (waitForFences && this->imagesInFlight[commandBufferIndex] != VK_NULL_HANDLE) {
        ret = vkResetFences(this->device, 1, &this->inFlightFences[this->currentFrame]);
        if (ret != VK_SUCCESS) {
            std::cerr << "Failed to Reset Fence!" << std::endl;
        }
    }
}
    
void Graphics::drawFrame() {    
    std::chrono::high_resolution_clock::time_point frameStart = std::chrono::high_resolution_clock::now();
    
    VkResult ret = vkWaitForFences(device, 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);
    if (ret != VK_SUCCESS) {
        std::cerr << "vkWaitForFences Failed" << std::endl;
    }
    
    if (this->requiresUpdateSwapChain) {
        this->updateSwapChain();
        this->requiresUpdateSwapChain = false;
    }
    
    uint32_t imageIndex;
    ret = vkAcquireNextImageKHR(
            this->device, this->swapChain, UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &imageIndex);
    

    if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
        this->updateSwapChain();
        return;
    } else if (ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to Acquire Swap Chain Image" << std::endl;
        this->currentFrame = (this->currentFrame + 1) % this->swapChainImages.size();
        return;
    }

    this->updateUniformBuffer(imageIndex);

    if (this->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &this->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    this->imagesInFlight[imageIndex] = this->inFlightFences[this->currentFrame];

    if (this->components.isSceneUpdateNeeded()) {
        this->updateScene(imageIndex);
    }
        
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {this->imageAvailableSemaphores[this->currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = this->context.commandBuffers.empty() ? 0 : 1;
    submitInfo.pCommandBuffers = &this->context.commandBuffers[imageIndex];

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

    if (ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR) {
        this->updateSwapChain();
    } else if (ret != VK_SUCCESS) {
       std::cerr << "Failed to Present Swap Chain Image!" << std::endl;
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

bool Graphics::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t & memoryType) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            memoryType = i;
            return true;
        }
    }

    return false;
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
    
    if (this->hasSkybox) {
        if (!this->createTextureSampler(this->skyboxSampler, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)) return false;
        if (!this->createSkyboxDescriptorSetLayout()) return false;
                    
        if (!this->createSkyboxDescriptorSets()) return false;
    }
    
    return true;
}

ModelSummary Graphics::getModelsBufferSizes() {
    ModelSummary bufferSizes;
    
    std::map<std::string, std::vector<std::unique_ptr<Component>>> & allComponents = this->components.getComponents();
    for (auto & comps : allComponents) {
        auto & compsPerModel = comps.second;
        
        uint32_t compCount = compsPerModel.size();
        if (compCount == 0) continue;
        
        auto model = compsPerModel[0]->getModel();
        auto meshes = model->getMeshes();
        for (Mesh & mesh : meshes) {
            VkDeviceSize vertexSize = mesh.getVertices().size();
            VkDeviceSize indexSize = mesh.getIndices().size();

            bufferSizes.vertexBufferSize += vertexSize * sizeof(class Vertex);
            bufferSizes.indexBufferSize += indexSize * sizeof(uint32_t);
            bufferSizes.ssboBufferSize += meshes.size() * compCount * sizeof(struct MeshProperties);
        }
    }
    
    std::cout << "Vertex Buffer Size: " << bufferSizes.vertexBufferSize << std::endl;
    std::cout << "Index Buffer Size: " << bufferSizes.indexBufferSize << std::endl;
    std::cout << "SSBO Buffer Size: " << bufferSizes.ssboBufferSize << std::endl;
    
    return bufferSizes;
}

bool Graphics::createBuffersFromModel(bool makeSsboBufferHostWritable) {
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
    if (!this->createSsboBufferFromModel(bufferSizes.ssboBufferSize, makeSsboBufferHostWritable)) return false;
        
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

void Graphics::drawSkybox(RenderContext & context, int commandBufferIndex) {    
    VkDeviceSize offsets[] = {0};
    VkBuffer vertexBuffers[] = {this->skyBoxVertexBuffer};
    vkCmdBindVertexBuffers(this->context.commandBuffers[commandBufferIndex], 0, 1, vertexBuffers, offsets);
    
    vkCmdDraw(context.commandBuffers[commandBufferIndex], SKYBOX_VERTICES.size(), 1, 0, 0);
}


void Graphics::draw(RenderContext & context, int commandBufferIndex, bool useIndices) {
    VkDeviceSize lastVertexOffset = 0;
    VkDeviceSize lastIndexOffset = 0;

    std::map<std::string, std::vector<std::unique_ptr<Component>>> & allComponents = this->components.getComponents();
    int c = 0;
    
    for (auto & comps : allComponents) {
        auto & compsPerModel = comps.second;
        
        uint32_t compCount = compsPerModel.size();
        if (compCount == 0) continue;
        
        auto model = compsPerModel[0]->getModel();
        if (model == nullptr) continue;
        
        auto meshes = model->getMeshes();
        for (Mesh & mesh : meshes) {
            VkDeviceSize vertexSize = mesh.getVertices().size();
            VkDeviceSize indexSize = mesh.getIndices().size();
            
            for (auto & comp : compsPerModel) {
                ModelProperties props = { comp->getModelMatrix()};
                
                vkCmdPushConstants(
                    context.commandBuffers[commandBufferIndex], context.graphicsPipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(struct ModelProperties), &props);

                if (useIndices) {                
                    vkCmdDrawIndexed(context.commandBuffers[commandBufferIndex], indexSize , 1, lastIndexOffset, lastVertexOffset, c);
                } else {
                    vkCmdDraw(context.commandBuffers[commandBufferIndex], vertexSize, 1, 0, 0);
                }
                c++;
            }
                        
            lastIndexOffset += indexSize;
            lastVertexOffset += vertexSize;
        }
    }
}

void Graphics::copyModelsContentIntoBuffer(void* data, ModelsContentType modelsContentType, VkDeviceSize maxSize) {
    VkDeviceSize overallSize = 0;
    
    std::map<std::string, std::vector<std::unique_ptr<Component>>> & allComponents = this->components.getComponents();
    for (auto & comps : allComponents) {
        auto & compsPerModel = comps.second;
        
        uint32_t compCount = compsPerModel.size();
        if (compCount == 0) continue;
        
        auto model = compsPerModel[0]->getModel();
        
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
                    int i=0;
                    for (auto & c : compsPerModel) {
                        MeshProperties modelProps = { 
                            textureInfo.ambientTexture,
                            textureInfo.diffuseTexture,
                            textureInfo.specularTexture,
                            textureInfo.normalTexture
                        };
                        dataSize = sizeof(struct MeshProperties);             
                        if (overallSize + dataSize <= maxSize) {
                            memcpy(static_cast<char *>(data)+overallSize, &modelProps, dataSize);
                            overallSize += dataSize;
                        }
                        if (c->getSsboIndex() == -1) {
                            c->setSsboIndex(i);
                        }
                        c->markSsboAsNotDirty();
                        i++;
                    }
                    break;
            }
        }
    }
}

Component * Graphics::addModelComponent(std::string modelLocation) {
    Model * model = this->models.findModelByLocation(modelLocation);
    if (model == nullptr) return nullptr;
    
    return this->components.addComponent(new Component(model));
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

bool Graphics::findDepthFormat(VkFormat & supportedFormat) {
    return this->findSupportedFormat(
    {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, supportedFormat
    );
}
    
bool Graphics::findSupportedFormat(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat & supportedFormat) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(this->physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            supportedFormat = format;
            return true;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            supportedFormat = format;
            return true;
        }
    }

    return false;
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


Graphics::~Graphics() {
    this->cleanupSwapChain();

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
        if (this->renderFinishedSemaphores[i] != nullptr) {
            vkDestroySemaphore(this->device, this->renderFinishedSemaphores[i], nullptr);
        }
        if (this->imageAvailableSemaphores[i] != nullptr) {
            vkDestroySemaphore(this->device, this->imageAvailableSemaphores[i], nullptr);
        }
        if (this->inFlightFences[i] != nullptr) {
            vkDestroyFence(this->device, this->inFlightFences[i], nullptr);
        }
    }

    if (this->device != nullptr && this->commandPool != nullptr) {
        vkDestroyCommandPool(this->device, this->commandPool, nullptr);
    }

    if (this->device != nullptr) vkDestroyDevice(this->device, nullptr);

    if (this->vkSurface != nullptr) vkDestroySurfaceKHR(this->vkInstance, this->vkSurface, nullptr);

    if (this->vkInstance != nullptr) vkDestroyInstance(this->vkInstance, nullptr);

    if (this->sdlWindow != nullptr) SDL_DestroyWindow(this->sdlWindow);

    SDL_Quit();
}

RenderContext & Graphics::getRenderContext() {
    return this->context;
}

Models & Graphics::getModels() {
    return this->models;
}

Components & Graphics::getComponents() {
    return this->components;
}

void Graphics::prepareComponents() {
    this->components.initWithModelLocations(this->models.getModelLocations());
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
