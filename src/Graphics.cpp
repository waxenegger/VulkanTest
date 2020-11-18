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
        if (!SDL_Vulkan_CreateSurface(this->sdlWindow, this->vkInstance, &this->vkSurface)) {
            std::cerr << "SDL Vulkan Surface could not be created! Error: " << SDL_GetError() << std::endl;
            return;
        }

        this->active = true;
    }
}

bool Graphics::initVulkan(const std::string & appName, uint32_t version) {
    this->queryVkExtensions();
    this->queryVkLayerProperties();

    this->createVkInstance(appName, version);
    if (this->vkInstance == nullptr) return false;

    this->queryVkPhysicalDevices();

    return true;
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
    app.pNext = NULL,
    app.pApplicationName = appName.c_str();
    app.applicationVersion = version;
    app.pEngineName = appName.c_str();
    app.engineVersion = version;
    app.apiVersion = VK_API_VERSION_1_0;

    this->queryVkExtensions();

    VkInstanceCreateInfo inst_info;
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app;
    inst_info.enabledLayerCount = 0;
    inst_info.ppEnabledLayerNames = NULL;
    inst_info.enabledExtensionCount = this->vkExtensionNames.size();
    inst_info.ppEnabledExtensionNames = !this->vkExtensionNames.empty() ? this->vkExtensionNames.data() : NULL;

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

void Graphics::queryVkLayerProperties() {
    uint32_t layerCount = 0;
    VkResult ret;

    ret = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    ASSERT_VULKAN(ret);

    if (ret == VK_SUCCESS) {
        this->vkLayerProperties.resize(layerCount);
        ret = vkEnumerateInstanceLayerProperties(&layerCount, this->vkLayerProperties.data());
        ASSERT_VULKAN(ret);
    }
}

const std::vector<VkQueueFamilyProperties> Graphics::getVkPhysicalDeviceQueueFamilyProperties(const unsigned int index) {
    if (index > this->vkPhysicalDevices.size()-1) return std::vector<VkQueueFamilyProperties>();

    uint32_t numberOfDeviceQueuerFamilyProperties = 0;

    vkGetPhysicalDeviceQueueFamilyProperties(this->vkPhysicalDevices[index], &numberOfDeviceQueuerFamilyProperties, nullptr);

    std::vector<VkQueueFamilyProperties> vkQueueFamilyProperties(numberOfDeviceQueuerFamilyProperties);
    vkGetPhysicalDeviceQueueFamilyProperties(this->vkPhysicalDevices[index], &numberOfDeviceQueuerFamilyProperties, vkQueueFamilyProperties.data());

    return vkQueueFamilyProperties;
}


void Graphics::listVkLayerProperties() {
    if (this->vkLayerProperties.empty()) return;

    std::cout << "Layers: " << std::endl;
    for (auto & layerProperties : this->vkLayerProperties) {
        std::cout << "\t" << layerProperties.layerName << std::endl;
    }
}

void Graphics::listVkPhysicalDeviceQueueFamilyProperties(std::vector<VkQueueFamilyProperties> & deviceQueueFamilyProperties) {
    int c = 0;
    for (auto & queueFamilyProperties : deviceQueueFamilyProperties) {
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
    /*
    vkDeviceWaitIdle(device) // wait till work is done
    vkDestroyDevice(device, pAllocator); // logical device
    */

    if (this->vkSurface != nullptr) vkDestroySurfaceKHR(this->vkInstance, this->vkSurface, nullptr);
    if (this->vkInstance != nullptr) vkDestroyInstance(this->vkInstance, nullptr);
    if (this->sdlWindow != nullptr) SDL_DestroyWindow(this->sdlWindow);
    SDL_Quit();
}

