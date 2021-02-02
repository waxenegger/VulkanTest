#ifndef SRC_INCLUDES_GRAPHICS_H_
#define SRC_INCLUDES_GRAPHICS_H_

#include "shared.h"
#include "Utils.hpp"

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
        VkSwapchainKHR swapChain = nullptr;

        Graphics();
        void initSDL();
        bool initVulkan(const std::string & appName, uint32_t version);

        void queryVkInstanceExtensions();

        std::vector<VkExtensionProperties> queryPhysicalDeviceExtensions(const VkPhysicalDevice & device);
        void listPhysicalDeviceExtensions(const VkPhysicalDevice & device);
        bool doesPhysicalDeviceSupportExtension(const VkPhysicalDevice & device, const std::string extension);

        std::vector<VkSurfaceFormatKHR> queryPhysicalDeviceSurfaceFormats(const VkPhysicalDevice & device);
        void listPhysicalDeviceSurfaceFormats(const VkPhysicalDevice & device);
        bool isPhysicalDeviceSurfaceFormatsSupported(const VkPhysicalDevice & device, const VkSurfaceFormatKHR & format);

        std::vector<VkPresentModeKHR> queryDeviceSwapModes(const VkPhysicalDevice & device);
        VkPresentModeKHR pickBestDeviceSwapMode(const std::vector<VkPresentModeKHR> & availableSwapModes);

        void createVkInstance(const std::string & appName, uint32_t version);

        void queryPhysicalDevices();

        void listVkLayerNames();

        std::tuple<int,int> ratePhysicalDevice(const VkPhysicalDevice & device);
        const std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device);
        VkPhysicalDevice createLogicalDeviceAndQueues();
        std::tuple<VkPhysicalDevice, int> pickBestPhysicalDeviceAndQueueIndex();
        bool getSurfaceCapabilities(const VkPhysicalDevice & physicalDevice, VkSurfaceCapabilitiesKHR & surfaceCapabilities);
        bool getSwapExtent(VkSurfaceCapabilitiesKHR & surfaceCapabilities, VkExtent2D & swapExtent);
        bool createSwapChain(const VkPhysicalDevice & physicalDevice);
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
