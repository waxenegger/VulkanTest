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
    const VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pApplicationName = appName.c_str(),
        .applicationVersion = version,
        .pEngineName = appName.c_str(),
        .engineVersion = version,
        .apiVersion = VK_API_VERSION_1_0,
    };

    this->queryVkExtensions();

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app;
    inst_info.enabledExtensionCount = this->vkExtensionNames.size();
    inst_info.ppEnabledExtensionNames = !this->vkExtensionNames.empty() ? this->vkExtensionNames.data() : nullptr;

    vkCreateInstance(&inst_info, nullptr, &this->vkInstance);
}

void Graphics::queryVkPhysicalDevices() {
    uint32_t physicalDeviceCount = 0;

    vkEnumeratePhysicalDevices(this->vkInstance, &physicalDeviceCount, nullptr);

    this->vkPhysicalDevices.resize(physicalDeviceCount);
    vkEnumeratePhysicalDevices(this->vkInstance, &physicalDeviceCount, this->vkPhysicalDevices.data());
}

void Graphics::listVkPhysicalDevices() {
    VkPhysicalDeviceProperties physicalProperties = {};
    for (auto & device : this->vkPhysicalDevices) {
        vkGetPhysicalDeviceProperties(device, &physicalProperties);
        std::cout << physicalProperties.deviceName << std::endl;
    }
}

void Graphics::listVkExtensions() {
    for (auto & extension : this->vkExtensionNames) {
        std::cout << extension << std::endl;
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
    if (this->vkInstance != nullptr) vkDestroyInstance(this->vkInstance, nullptr);
    if (this->sdlWindow != nullptr) SDL_DestroyWindow(this->sdlWindow);
    SDL_Quit();
}

