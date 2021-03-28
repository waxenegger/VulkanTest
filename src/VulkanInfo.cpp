#include "includes/graphics.h"

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
