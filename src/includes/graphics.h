#ifndef SRC_INCLUDES_GRAPHICS_H_
#define SRC_INCLUDES_GRAPHICS_H_

#include "models.h"
#include "utils.h"


const int MAX_FRAMES_IN_FLIGHT = 2;

class Graphics final {
    private:
        SDL_Window * sdlWindow = nullptr;
        VkInstance vkInstance = nullptr;
        VkSurfaceKHR vkSurface = nullptr;
        bool active = false;

        std::vector<const char *> vkExtensionNames;
        std::vector<VkPhysicalDevice> vkPhysicalDevices;
        std::vector<const char *> vkLayerNames = {
            // "VK_LAYER_KHRONOS_validation"
        };

        bool showWireFrame = false;
        
        VkPhysicalDevice physicalDevice = nullptr;
        VkDevice device = nullptr;

        uint32_t graphicsQueueIndex = -1;
        VkQueue graphicsQueue = nullptr;
        uint32_t presentQueueIndex = -1;
        VkQueue presentQueue = nullptr;

        VkSwapchainKHR swapChain = nullptr;
        std::vector<VkImage> swapChainImages;
        VkSurfaceFormatKHR swapChainImageFormat = {
                VK_FORMAT_B8G8R8A8_SRGB,
                VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
        };
        VkExtent2D swapChainExtent;

        std::vector<VkImageView> swapChainImageViews;

        VkCommandPool commandPool = nullptr;
        VkDescriptorPool descriptorPool = nullptr;

        std::vector<VkDescriptorSet> descriptorSets;
        
        VkRenderPass renderPass = nullptr;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipeline graphicsPipeline = nullptr;

        std::vector<VkFramebuffer> swapChainFramebuffers;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;

        size_t currentFrame = 0;
        bool framebufferResized = false;

        VkBuffer vertexBuffer = nullptr;
        VkDeviceMemory vertexBufferMemory = nullptr;

        VkBuffer indexBuffer = nullptr;
        VkDeviceMemory indexBufferMemory = nullptr;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        
        Models models;
        
        RenderContext context;

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

        std::vector<VkPresentModeKHR> queryDeviceSwapModes();
        VkPresentModeKHR pickBestDeviceSwapMode(const std::vector<VkPresentModeKHR> & availableSwapModes);

        bool createImageViews();

        void createVkInstance(const std::string & appName, uint32_t version);

        bool createGraphicsPipeline();
        VkShaderModule createShaderModule(const std::vector<char> & code);

        void queryPhysicalDevices();

        std::tuple<int,int> ratePhysicalDevice(const VkPhysicalDevice & device);
        const std::vector<VkQueueFamilyProperties> getPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device);
        bool createLogicalDeviceAndQueues();
        std::tuple<VkPhysicalDevice, int> pickBestPhysicalDeviceAndQueueIndex();
        bool getSurfaceCapabilities(VkSurfaceCapabilitiesKHR & surfaceCapabilities);
        bool getSwapChainExtent(VkSurfaceCapabilitiesKHR & surfaceCapabilities);
        bool createSwapChain();
        bool createDescriptorPool();
        bool createDescriptorSetLayout();
        bool createDescriptorSets();
        
        bool createUniformBuffers();
        void updateUniformBuffer(uint32_t currentImage);

        void listVkPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device);

        bool createCommandPool();
        bool createFramebuffers();
        bool createCommandBuffers();
        bool createRenderPass();

        bool createSyncObjects();

        void cleanupSwapChain();

        bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        bool findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t & memoryType);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        bool createBuffersFromModel();
        
    public:
        Graphics(const Graphics&) = delete;
        Graphics& operator=(const Graphics &) = delete;
        Graphics(Graphics &&) = delete;
        Graphics & operator=(Graphics) = delete;

        static Graphics & instance();
        bool isActive();
        void init(const std::string & appName, uint32_t version);

        void listVkExtensions();
        void listVkLayerNames();
        void listVkPhysicalDevices();

        bool updateSwapChain();
        void drawFrame();
        void addModel(Model & model);
        RenderContext & getRenderContext();
        void renderScene();
        void toggleWireFrame();
        
        std::tuple<int, int> getWindowSize();
        Models & getModels();

        ~Graphics();
};

#endif
