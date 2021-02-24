#include "includes/graphics.h"

Graphics::Graphics() {
    this->initSDL();
}

void Graphics::initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "Could not initialize SDL! Error: " << SDL_GetError() << std::endl;
        return;
    }

    this->sdlWindow =
            SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (this->sdlWindow == nullptr) {
        std::cerr << "SDL Window could not be created! Error: " << SDL_GetError() << std::endl;
        return;
    }
}

void Graphics::init(const std::string & appName, uint32_t version) {
    if (this->sdlWindow == nullptr) {
        return;
    }

    SDL_SetWindowTitle(this->sdlWindow, appName.c_str());
    SDL_SetRelativeMouseMode(SDL_TRUE);

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
    if (!this->createDescriptorPool()) return false;
    
    if (!this->createUniformBuffers()) return false;
    
    if (!this->createSyncObjects()) return false;

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
        auto windowSize = this->getWindowSize();
        int width = std::get<0>(windowSize);
        int height = std::get<1>(windowSize);
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

std::tuple< int, int > Graphics::getWindowSize()
{
    if (!this->isActive()) return std::make_tuple(0,0);
    
    int width, height;
    SDL_Vulkan_GetDrawableSize(this->sdlWindow, &width, &height);
    return std::make_tuple(width, height);
};

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

    uint32_t imageCount = surfaceCapabilities.minImageCount;
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
        createInfo.queueFamilyIndexCount = 2;
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

    this->swapChainImages.resize(imageCount);

    ret = vkGetSwapchainImagesKHR(device, swapChain, &imageCount, this->swapChainImages.data());
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Swap Chain Images!" << std::endl;
        return false;
    }

    return true;
}

VkImageView Graphics::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VkResult ret = vkCreateImageView(this->device, &viewInfo, nullptr, &imageView);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Image View!" << std::endl;
        return nullptr;
    }

    return imageView;
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

    const std::vector<const char * > extensionsToEnable = { "VK_KHR_swapchain" };

    VkPhysicalDeviceFeatures deviceFeatures {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

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

void Graphics::listPhysicalDeviceExtensions(const VkPhysicalDevice & device) {
    std::vector<VkExtensionProperties> availableExtensions = this->queryPhysicalDeviceExtensions(device);

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
     if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
         throw std::runtime_error("failed to create shader module!");
     }

     return shaderModule;
}

bool Graphics::createGraphicsPipeline() {
    // TODO: modify, to optimize
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    if (!Utils::readFile("/opt/projects/VulkanTest/res/shaders/vert.spv", vertShaderCode) ||
            !Utils::readFile("/opt/projects/VulkanTest/res/shaders/frag.spv", fragShaderCode)) {
        std::cerr << "Failed to read shader files" << std::endl;
        return false;
    }

    VkShaderModule vertShaderModule = this->createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = this->createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (this->createBuffersFromModel()) {
        const VkVertexInputBindingDescription & bindingDescription = Vertex::getBindingDescription();
        const std::array<VkVertexInputAttributeDescription, 2> & attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    } else {
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
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
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
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

    vkDestroyShaderModule(this->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(this->device, vertShaderModule, nullptr);

    return true;
 }

bool Graphics::createFramebuffers() {
     this->swapChainFramebuffers.resize(this->swapChainImageViews.size());

     for (size_t i = 0; i < this->swapChainImageViews.size(); i++) {
         std::array<VkImageView, 2> attachments = {
             this->swapChainImageViews[i], this->depthImageView
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
    VkDescriptorPoolSize poolSize {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(this->swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(this->swapChainImages.size());

    VkResult ret = vkCreateDescriptorPool(device, &poolInfo, nullptr, &this->descriptorPool);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
       std::cerr << "Failed to Create Descriptor Pool!" << std::endl;
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
    VkDescriptorSetLayoutBinding modelUniformLayoutBinding{};
    modelUniformLayoutBinding.binding = 0;
    modelUniformLayoutBinding.descriptorCount = 1;
    modelUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelUniformLayoutBinding.pImmutableSamplers = nullptr;
    modelUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &modelUniformLayoutBinding;

    VkResult ret = vkCreateDescriptorSetLayout(this->device, &layoutInfo, nullptr, &this->descriptorSetLayout);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Create Descriptor Set Layout!" << std::endl;
        return false;
    }
    
    return true;
}

bool Graphics::createDescriptorSets() {
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
    
    for (size_t i = 0; i < this->descriptorSets.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = this->uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(struct ModelUniforms);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = this->descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
    }
    
    return true;
}

bool Graphics::createCommandBuffers() {
        // TODO reset command buffers ?
        this->context.commandBuffers.resize(this->swapChainFramebuffers.size());

       VkCommandBufferAllocateInfo allocInfo{};
       allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
       allocInfo.commandPool = this->commandPool;
       allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
       allocInfo.commandBufferCount = (uint32_t) this->context.commandBuffers.size();

       VkResult ret = vkAllocateCommandBuffers(device, &allocInfo, this->context.commandBuffers.data());
       if (ret != VK_SUCCESS) {
          std::cerr << "Failed to Allocate Command Buffers!" << std::endl;
          return false;
       }

       for (size_t i = 0; i < this->context.commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            ret = vkBeginCommandBuffer(this->context.commandBuffers[i], &beginInfo);
            if (ret != VK_SUCCESS) {
                std::cerr << "Failed to begin Recording Command Buffer!" << std::endl;
                return false;
            }

           VkRenderPassBeginInfo renderPassInfo{};
           renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
           renderPassInfo.renderPass = this->renderPass;
           renderPassInfo.framebuffer = this->swapChainFramebuffers[i];
           renderPassInfo.renderArea.offset = {0, 0};
           renderPassInfo.renderArea.extent = this->swapChainExtent;

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
            clearValues[1].depthStencil = {1.0f, 0};

           renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
           renderPassInfo.pClearValues = clearValues.data();

           vkCmdBeginRenderPass(this->context.commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
           
           vkCmdBindPipeline(this->context.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, this->graphicsPipeline);
           
           if (this->vertexBuffer != nullptr) {
               VkBuffer vertexBuffers[] = {this->vertexBuffer};
               VkDeviceSize offsets[] = {0};
               vkCmdBindVertexBuffers(this->context.commandBuffers[i], 0, 1, vertexBuffers, offsets);
           }
           
           vkCmdBindDescriptorSets(
               this->context.commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, 
               this->context.graphicsPipelineLayout, 0, 1, &this->descriptorSets[i], 0, nullptr);
           
            if (this->indexBuffer != nullptr) {
                vkCmdBindIndexBuffer(this->context.commandBuffers[i], this->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
                this->models.draw(this->context, i, true);
            } else {
                this->models.draw(this->context, i, false);                
            }


           vkCmdEndRenderPass(this->context.commandBuffers[i]);

           ret = vkEndCommandBuffer(this->context.commandBuffers[i]);
           if (ret != VK_SUCCESS) {
               std::cerr << "Failed to end  Recording Command Buffer!" << std::endl;
               return false;
           }
       }

       return true;
   }

bool Graphics::updateSwapChain() {
    if (this->device == nullptr) return false;

    this->cleanupSwapChain();

    if (!this->createSwapChain()) return false;
    if (!this->createImageViews()) return false;
    if (!this->createRenderPass()) return false;
    if (!this->createGraphicsPipeline()) return false;
    if (!this->createDepthResources()) return false;
    if (!this->createFramebuffers()) return false;
    if (!this->createCommandBuffers()) return false;

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
            
    if (this->depthImageView != nullptr) vkDestroyImageView(this->device, this->depthImageView, nullptr);
    if (this->depthImage != nullptr) vkDestroyImage(this->device, this->depthImage, nullptr);
    if (this->depthImageMemory != nullptr) vkFreeMemory(this->device, this->depthImageMemory, nullptr);

    for (auto framebuffer : this->swapChainFramebuffers) {
        vkDestroyFramebuffer(this->device, framebuffer, nullptr);
    }

    if (this->commandPool != nullptr) {
        vkFreeCommandBuffers(
            this->device, this->commandPool, static_cast<uint32_t>(this->context.commandBuffers.size()), this->context.commandBuffers.data());
    }

    if (this->graphicsPipeline != nullptr) vkDestroyPipeline(this->device, this->graphicsPipeline, nullptr);
    if (this->context.graphicsPipelineLayout != nullptr) vkDestroyPipelineLayout(this->device, this->context.graphicsPipelineLayout, nullptr);
    if (this->renderPass != nullptr) vkDestroyRenderPass(this->device, this->renderPass, nullptr);

    for (auto imageView : this->swapChainImageViews) {
        vkDestroyImageView(this->device, imageView, nullptr);
    }

    if (this->swapChain != nullptr) vkDestroySwapchainKHR(this->device, this->swapChain, nullptr);
}

bool Graphics::createCommandPool() {
    if (this->device == nullptr) return false;
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
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
    modelUniforms.viewMatrix = Camera::instance()->getViewMatrix();
    modelUniforms.projectionMatrix = Camera::instance()->getProjectionMatrix();

    void* data;
    vkMapMemory(this->device, this->uniformBuffersMemory[currentImage], 0, sizeof(modelUniforms), 0, &data);
    memcpy(data, &modelUniforms, sizeof(modelUniforms));
    vkUnmapMemory(this->device, this->uniformBuffersMemory[currentImage]);
}

void Graphics::renderScene() {
    vkDeviceWaitIdle(this->device);
    
    vkFreeCommandBuffers(
        this->device, this->commandPool, static_cast<uint32_t>(this->context.commandBuffers.size()), this->context.commandBuffers.data());

    this->createCommandBuffers();
}
    
void Graphics::drawFrame() {
    VkResult ret = vkWaitForFences(device, 1, &this->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    if (ret != VK_SUCCESS) {
        std::cerr << "vkWaitForFences Failed" << std::endl;
        return;
    }

    uint32_t imageIndex;
    ret = vkAcquireNextImageKHR(
            this->device, this->swapChain, UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (ret == VK_ERROR_OUT_OF_DATE_KHR) {
        this->updateSwapChain();
        return;
    } else if (ret != VK_SUCCESS && ret != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to Acquire Swap Chain Image" << std::endl;
        return;
    }
    
    this->updateUniformBuffer(imageIndex);
    
    if (this->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &this->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    this->imagesInFlight[imageIndex] = this->inFlightFences[this->currentFrame];

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

    vkResetFences(this->device, 1, &this->inFlightFences[this->currentFrame]);

    if (vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, this->inFlightFences[this->currentFrame]) != VK_SUCCESS) {
        std::cerr << "Failed to Submit Draw Command Buffer!" << std::endl;
        return;
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

    if (ret == VK_ERROR_OUT_OF_DATE_KHR || ret == VK_SUBOPTIMAL_KHR || this->framebufferResized) {
        this->framebufferResized = false;
        this->updateSwapChain();
    } else if (ret != VK_SUCCESS) {
       std::cerr << "Failed to Present Swap Chain Image!" << std::endl;
       return;
    }

    this->currentFrame = (this->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Graphics::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = this->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(this->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(this->graphicsQueue);

    vkFreeCommandBuffers(this->device, this->commandPool, 1, &commandBuffer);
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
    if (!this->createDescriptorSetLayout()) return false;
    
    VkDeviceSize bufferSize = sizeof(struct ModelUniforms);

    this->uniformBuffers.resize(this->swapChainImages.size());
    this->uniformBuffersMemory.resize(this->swapChainImages.size());

    for (size_t i = 0; i < this->swapChainImages.size(); i++) {
        this->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                        this->uniformBuffers[i], this->uniformBuffersMemory[i]);
    }
    
    if (!this->createDescriptorSets()) return false;
    
    return true;
}

bool Graphics::createBuffersFromModel() {
    if (this->models.getTotalNumberOfVertices() == 0) return true;

    if (this->vertexBuffer != nullptr) vkDestroyBuffer(this->device, this->vertexBuffer, nullptr);
    if (this->vertexBufferMemory != nullptr) vkFreeMemory(this->device, this->vertexBufferMemory, nullptr);

    VkDeviceSize bufferSize = sizeof(class Vertex) * this->models.getTotalNumberOfVertices();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    if (!this->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory)) {
        std::cerr << "Failed to get Create Staging Buffer" << std::endl;
        return false;
    }

    void* data = nullptr;
    vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    this->models.copyModelsContentIntoBuffer(data, false, bufferSize);
    vkUnmapMemory(this->device, stagingBufferMemory);

    if (!this->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->vertexBuffer, this->vertexBufferMemory)) {
        std::cerr << "Failed to get Create Vertex Buffer" << std::endl;
        return false;
    }

    this->copyBuffer(stagingBuffer,this->vertexBuffer, bufferSize);

    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);
    
    // indices
    if (this->models.getTotalNumberOfIndices() == 0) return true;

    if (this->indexBuffer != nullptr) vkDestroyBuffer(this->device, this->indexBuffer, nullptr);
    if (this->indexBufferMemory != nullptr) vkFreeMemory(this->device, this->indexBufferMemory, nullptr);

    bufferSize = sizeof(uint32_t) * this->models.getTotalNumberOfIndices();

    if (!this->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer, stagingBufferMemory)) {
        std::cerr << "Failed to get Create Staging Buffer" << std::endl;
        return false;
    }

    data = nullptr;
    vkMapMemory(this->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    this->models.copyModelsContentIntoBuffer(data, true, bufferSize);
    vkUnmapMemory(this->device, stagingBufferMemory);

    if (!this->createBuffer(
            bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            this->indexBuffer, this->indexBufferMemory)) {
        std::cerr << "Failed to get Create Vertex Buffer" << std::endl;
        return false;
    }

    this->copyBuffer(stagingBuffer,this->indexBuffer, bufferSize);

    vkDestroyBuffer(this->device, stagingBuffer, nullptr);
    vkFreeMemory(this->device, stagingBufferMemory, nullptr);

    return true;
}

bool Graphics::createImage(
    int32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    if (!this->findDepthFormat(depthFormat)) {
    
        return false;
    };

    this->createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = this->createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    
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

bool Graphics::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
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
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
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

void Graphics::addModel(Model & model) {
    if (model.hasBeenLoaded()) this->models.addModel(model);
}

void Graphics::toggleWireFrame() {
    this->showWireFrame = !this->showWireFrame;
    
    this->updateSwapChain();
}

Graphics::~Graphics() {
    this->cleanupSwapChain();

    if (this->descriptorPool != nullptr) {
        vkDestroyDescriptorPool(this->device, this->descriptorPool, nullptr);
    }
    
    if (this->descriptorSetLayout != nullptr) {
        vkDestroyDescriptorSetLayout(this->device, this->descriptorSetLayout, nullptr);
    }

    if (this->vertexBuffer != nullptr) vkDestroyBuffer(this->device, this->vertexBuffer, nullptr);
    if (this->vertexBufferMemory != nullptr) vkFreeMemory(this->device, this->vertexBufferMemory, nullptr);

    if (this->indexBuffer != nullptr) vkDestroyBuffer(this->device, this->indexBuffer, nullptr);
    if (this->indexBufferMemory != nullptr) vkFreeMemory(this->device, this->indexBufferMemory, nullptr);

    for (size_t i = 0; i < this->uniformBuffers.size(); i++) {
        vkDestroyBuffer(this->device, this->uniformBuffers[i], nullptr);
    }
    for (size_t i = 0; i < this->uniformBuffersMemory.size(); i++) {
        vkFreeMemory(this->device, this->uniformBuffersMemory[i], nullptr);
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(this->device, this->renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(this->device, this->imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(this->device, this->inFlightFences[i], nullptr);
    }

    if (this->device != nullptr && this->commandPool != nullptr) {
        vkDestroyCommandPool(this->device, this->commandPool, nullptr);
    }

    if (this->device != nullptr) vkDestroyDevice(this->device, nullptr);

    if (this->vkSurface != nullptr) vkDestroySurfaceKHR(this->vkInstance, this->vkSurface, nullptr);

    if (this->vkInstance != nullptr) vkDestroyInstance(this->vkInstance, nullptr);

    if (this->sdlWindow != nullptr) SDL_DestroyWindow(this->sdlWindow);

    SDL_Quit();

    this->models.clear();
}

RenderContext & Graphics::getRenderContext() {
    return this->context;
}

Models & Graphics::getModels() {
    return this->models;
}


