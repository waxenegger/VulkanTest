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
        bool active = false;

        std::vector<const char *> vkExtensionNames;
        std::vector<VkPhysicalDevice> vkPhysicalDevices;
        std::vector<VkLayerProperties> vkLayerProperties;

        Graphics();
        void initSDL();
        bool initVulkan(const std::string & appName, uint32_t version);

        void queryVkExtensions();
        void createVkInstance(const std::string & appName, uint32_t version);
        void queryVkPhysicalDevices();
        void queryVkLayerProperties();
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
        void listVkLayerProperties();
        const std::vector<VkQueueFamilyProperties> getVkPhysicalDeviceQueueFamilyProperties(const unsigned int index = 0);
        void listVkPhysicalDeviceQueueFamilyProperties(std::vector<VkQueueFamilyProperties> & deviceQueueFamilyProperties);

        ~Graphics();
};

#endif
