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
    }
}

void Graphics::init(const std::string & appName, uint32_t version) {
    if (this->sdlWindow == nullptr) {
        return;
    }

    SDL_SetWindowTitle(this->sdlWindow, appName.c_str());

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

    VkPhysicalDevice physicalDevice = this->createLogicalDeviceAndQueues();
    if (physicalDevice == nullptr) return false;

    if (!this->createSwapChain(physicalDevice)) return false;


    return true;
}

bool Graphics::getSurfaceCapabilities(const VkPhysicalDevice & physicalDevice, VkSurfaceCapabilitiesKHR & surfaceCapabilities) {
    if (this->sdlWindow == nullptr || this->vkSurface == nullptr) return false;

    const VkResult ret = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, this->vkSurface, &surfaceCapabilities);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to get Device Surface Capabilites" << std::endl;
        return false;
    }

    return true;
}

bool Graphics::getSwapExtent(VkSurfaceCapabilitiesKHR & surfaceCapabilities, VkExtent2D & swapExtent) {
    if (this->sdlWindow == nullptr || this->vkSurface == nullptr) return false;

    if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
        swapExtent.width = surfaceCapabilities.currentExtent.width;
        swapExtent.height = surfaceCapabilities.currentExtent.height;

        return true;
    } else {
        int width, height;
        SDL_Vulkan_GetDrawableSize(this->sdlWindow, &width, &height);

        swapExtent.width = std::max(
            surfaceCapabilities.minImageExtent.width,
            std::min(surfaceCapabilities.maxImageExtent.width,
            static_cast<uint32_t>(width)));
        swapExtent.height = std::max(
            surfaceCapabilities.minImageExtent.height,
            std::min(surfaceCapabilities.maxImageExtent.height,
            static_cast<uint32_t>(height)));

        return true;
    }
}

bool Graphics::createSwapChain(const VkPhysicalDevice & physicalDevice) {
    if (this->device == nullptr) return false;

    const std::vector<VkPresentModeKHR> presentModes = this->queryDeviceSwapModes(physicalDevice);
    if (presentModes.empty()) {
        std::cerr << "Swap Modes Require Surface!" << std::endl;
        return false;
    }

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    if (!this->getSurfaceCapabilities(physicalDevice, surfaceCapabilities)) return false;

    VkExtent2D swapExtent;
    if (!this->getSwapExtent(surfaceCapabilities, swapExtent)) {
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
    createInfo.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = swapExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.preTransform = surfaceCapabilities.currentTransform;
    createInfo.presentMode = this->pickBestDeviceSwapMode(presentModes);
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    // TODO: revisit
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0; // Optional
    createInfo.pQueueFamilyIndices = nullptr; // Optional

    const VkResult ret = vkCreateSwapchainKHR(this->device, &createInfo, nullptr, &this->swapChain);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to Creat Swap Chain!" << std::endl;
        return false;
    }

    return true;
}

VkPhysicalDevice Graphics::createLogicalDeviceAndQueues() {
    // pick best physical device and queue index
    const std::tuple<VkPhysicalDevice, int> & bestPhysicalDeviceAndQueue = this->pickBestPhysicalDeviceAndQueueIndex();

    const VkPhysicalDevice bestPhysicalDevice = std::get<0>(bestPhysicalDeviceAndQueue);
    // could be that we don't have a graphics queue (unlikely but whatever)
    if (bestPhysicalDevice == nullptr) {
        std::cerr << "No adequate Physical Device found" << std::endl;
        return nullptr;
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

    const VkResult ret = vkCreateDevice(bestPhysicalDevice, &createInfo, nullptr, &this->device);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to create Logical Device!" << std::endl;
        return bestPhysicalDevice;
    }

    vkGetDeviceQueue(this->device, queueCreateInfos[0].queueFamilyIndex , 0, &this->graphicsQueue);
    vkGetDeviceQueue(this->device, queueCreateInfos[0].queueFamilyIndex , 0, &this->presentQueue);

    return bestPhysicalDevice;
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

std::vector<VkPresentModeKHR> Graphics::queryDeviceSwapModes(const VkPhysicalDevice & device) {
    uint32_t swapModesCount = 0;
    std::vector<VkPresentModeKHR> swapModes;

    if (this->vkSurface == nullptr) return swapModes;

    VkResult ret = vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->vkSurface, &swapModesCount, nullptr);
    ASSERT_VULKAN(ret);

    if (ret == VK_SUCCESS && swapModesCount > 0) {
        swapModes.resize(swapModesCount);
        ret = vkGetPhysicalDeviceSurfacePresentModesKHR(device, this->vkSurface, &swapModesCount, swapModes.data());
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
    VkSurfaceFormatKHR requiredFormat;
    requiredFormat.format = VK_FORMAT_B8G8R8A8_SRGB;
    requiredFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    // check if physical device supports swap chains and required surface format
    if (!this->doesPhysicalDeviceSupportExtension(device, "VK_KHR_swapchain") ||
            !this->isPhysicalDeviceSurfaceFormatsSupported(device, requiredFormat)) {
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

    this->listVkExtensions();

    this->listVkLayerNames();

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

Graphics::~Graphics() {
    vkDeviceWaitIdle(this->device);

    //if (this->swapChain != nullptr) vkDestroySwapchainKHR(this->device, this->swapChain, nullptr);

    if (this->device != nullptr) vkDestroyDevice(this->device, nullptr);

    //if (this->vkSurface != nullptr) vkDestroySurfaceKHR(this->vkInstance, this->vkSurface, nullptr);

    //if (this->vkInstance != nullptr) vkDestroyInstance(this->vkInstance, nullptr);

    if (this->sdlWindow != nullptr) SDL_DestroyWindow(this->sdlWindow);

    SDL_Quit();
}

