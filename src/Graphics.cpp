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
    this->createVkInstance(appName, version);
    if (this->vkInstance == nullptr) return false;

    this->queryVkPhysicalDevices();

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


    return true;
}

bool Graphics::createSwapChain() {
    return true;
}

bool Graphics::createLogicalDeviceAndQueues() {
    // pick best physical device and queue index
    const std::tuple<VkPhysicalDevice, int> & bestPhysicalDeviceAndQueue = this->pickBestPhysicalDeviceAndQueueIndex();

    const VkPhysicalDevice bestPhysicalDevice = std::get<0>(bestPhysicalDeviceAndQueue);
    // could be that we don't have a graphics queue (unlikely but whatever)
    if (bestPhysicalDevice == nullptr) {
        std::cerr << "Physical Device has no Graphics Queue" << std::endl;
        return false;
    }

    const int bestPhysicalQueueIndex = std::get<1>(bestPhysicalDeviceAndQueue);

    // create 2 queue infos for graphics and presentation (surface)
    const int nrOfQueues = 2;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(nrOfQueues);

    int x=0;
    while(x<nrOfQueues) {
        VkDeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.flags = 0;
        queueCreateInfo.pNext = nullptr;
        queueCreateInfo.queueFamilyIndex = bestPhysicalQueueIndex;
        queueCreateInfo.queueCount = 1; // we use only one for now
        const float priority = 1.0f;
        queueCreateInfo.pQueuePriorities = &priority;

        queueCreateInfos.push_back(queueCreateInfo);
        x++;
    }

    VkPhysicalDeviceFeatures deviceFeatures {};
    VkDeviceCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;

    const VkResult ret = vkCreateDevice(bestPhysicalDevice, &createInfo, nullptr, this->device);
    ASSERT_VULKAN(ret);
    if (ret != VK_SUCCESS) {
        std::cerr << "Failed to create Logical Device!" << std::endl;
        return false;
    }

    int y=0;
    while(y<nrOfQueues) {
        vkGetDeviceQueue(*this->device, queueCreateInfos[y].queueFamilyIndex , 0, this->presentQueue);
        y++;
    }

    return true;
}

std::tuple<VkPhysicalDevice, int> Graphics::pickBestPhysicalDeviceAndQueueIndex() {
    std::tuple<VkPhysicalDevice, int> choice = std::make_tuple(nullptr, -1);

    if (this->vkPhysicalDevices.empty()) return choice;

    int highestScore = 0;
    int i=0;
    for (auto & device : this->vkPhysicalDevices) {
        const std::tuple<int, int> & scoreAndQueueIndex = this->ratePhysicalDevice(device);
        if (std::get<0>(scoreAndQueueIndex) > highestScore && std::get<1>(scoreAndQueueIndex) != -1) {
            highestScore = std::get<0>(scoreAndQueueIndex);
            choice = std::make_tuple(device, std::get<1>(scoreAndQueueIndex));
        }
        i++;
    }

    return choice;
}

std::tuple<int,int> Graphics::ratePhysicalDevice(const VkPhysicalDevice & device) {
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

    const std::vector<VkQueueFamilyProperties> queueFamiliesProperties = this->getVkPhysicalDeviceQueueFamilyProperties(device);

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


void Graphics::queryVkExtensions() {
    uint32_t extensionCount = 0;
    if (SDL_Vulkan_GetInstanceExtensions(this->sdlWindow, &extensionCount, nullptr) == SDL_FALSE) {
        std::cerr << "Could not get SDL Vulkan Extensions: " << SDL_GetError() << std::endl;
    } else {
        this->vkExtensionNames.resize(extensionCount);
        SDL_Vulkan_GetInstanceExtensions(this->sdlWindow, &extensionCount, this->vkExtensionNames.data());
    }
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

    this->queryVkExtensions();
    this->listVkExtensions();

    //this->queryVkLayerNames();
    //this->listVkLayerNames();

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

void Graphics::queryVkPhysicalDevices() {
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

void Graphics::queryVkLayerNames() {
    uint32_t layerCount = 0;
    VkResult ret;

    ret = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    ASSERT_VULKAN(ret);

    std::vector<VkLayerProperties> availableLayers(layerCount);

    if (ret == VK_SUCCESS) {
        ret = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        ASSERT_VULKAN(ret);

        if (availableLayers.empty()) return;

        for (auto layer : availableLayers) {
            this->vkLayerNames.push_back(std::string(layer.layerName).c_str());
        }
    }
}

const std::vector<VkQueueFamilyProperties> Graphics::getVkPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device) {
    uint32_t numberOfDeviceQueuerFamilyProperties = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfDeviceQueuerFamilyProperties, nullptr);

    std::vector<VkQueueFamilyProperties> vkQueueFamilyProperties(numberOfDeviceQueuerFamilyProperties);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfDeviceQueuerFamilyProperties, vkQueueFamilyProperties.data());

    return vkQueueFamilyProperties;
}


void Graphics::listVkLayerNames() {
    if (this->vkLayerNames.empty()) return;

    std::cout << "Layers: " << std::endl;
    for (auto & layerName : this->vkLayerNames) {
        std::cout << "\t" << layerName << std::endl;
    }
}

void Graphics::listVkPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device) {
    const std::vector<VkQueueFamilyProperties> queueFamiliesProperties = this->getVkPhysicalDeviceQueueFamilyProperties(device);
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
    if (this->device != nullptr) {
        if (this->swapChain != nullptr) vkDestroySwapchainKHR(*this->device, *this->swapChain, nullptr);

        vkDestroyDevice(*this->device, nullptr); // logical device
    }

    if (this->vkSurface != nullptr) vkDestroySurfaceKHR(this->vkInstance, this->vkSurface, nullptr);
    if (this->vkInstance != nullptr) vkDestroyInstance(this->vkInstance, nullptr);

    if (this->sdlWindow != nullptr) SDL_DestroyWindow(this->sdlWindow);
    SDL_Quit();
}

