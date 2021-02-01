#ifndef SRC_INCLUDES_GRAPHICS_H_
#define SRC_INCLUDES_GRAPHICS_H_

#include "shared.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#define ASSERT_VULKAN(val) \
    if (val != VK_SUCCESS) { \
        std::cerr << "Error: " << val << std::endl; \
    };


class Graphics final {
    private:
        SDL_Window * sdlWindow = nullptr;
        VkInstance vkInstance = nullptr;
        VkSurfaceKHR vkSurface = nullptr;
        bool active = false;

        std::vector<const char *> vkExtensionNames;
        std::vector<VkPhysicalDevice> vkPhysicalDevices;
        std::vector<const char *> vkLayerNames = {
                "VK_LAYER_KHRONOS_validation"
        };

        VkDevice device = nullptr;
        VkQueue graphicsQueue = nullptr;
        VkQueue presentQueue = nullptr;
        VkSwapchainKHR * swapChain = nullptr;

        Graphics();
        void initSDL();
        bool initVulkan(const std::string & appName, uint32_t version);

        void queryVkExtensions();
        void createVkInstance(const std::string & appName, uint32_t version);
        void queryVkPhysicalDevices();
        void listVkLayerNames();
        std::tuple<int,int> ratePhysicalDevice(const VkPhysicalDevice & device);
        const std::vector<VkQueueFamilyProperties> getVkPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device);
        bool createLogicalDeviceAndQueues();
        std::tuple<VkPhysicalDevice, int> pickBestPhysicalDeviceAndQueueIndex();
        bool createSwapChain();
    public:
        Graphics(const Graphics&) = delete;
        Graphics& operator=(const Graphics &) = delete;
        Graphics(Graphics &&) = delete;
        Graphics & operator=(Graphics) = delete;

        static Graphics & instance();
        bool isActive();
        void init(const std::string & appName, uint32_t version);
        void listVkPhysicalDevices();
        void listVkExtensions();
        void listVkPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device);

        ~Graphics();
};

#endif
