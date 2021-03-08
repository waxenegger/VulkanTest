#ifndef SRC_INCLUDES_GRAPHICS_H_
#define SRC_INCLUDES_GRAPHICS_H_

#include "components.h"
#include "utils.h"

const int MAX_FRAMES_IN_FLIGHT = 2;
const int MAX_TEXTURES = 25;

struct ModelSummary {
    VkDeviceSize vertexBufferSize = 0;
    VkDeviceSize indexBufferSize = 0;
    VkDeviceSize ssboBufferSize = 0;
};

class Graphics final {
    private:
        SDL_Window * sdlWindow = nullptr;
        VkInstance vkInstance = nullptr;
        VkSurfaceKHR vkSurface = nullptr;
        bool active = false;
        
        std::string dir = "./";

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

        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfo;
        
        std::vector<VkFramebuffer> swapChainFramebuffers;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;

        size_t currentFrame = 0;
        bool framebufferResized = false;

        VkBuffer vertexBuffer = nullptr;
        VkDeviceMemory vertexBufferMemory = nullptr;
        
        VkShaderModule vertShaderModule = nullptr;
        VkShaderModule fragShaderModule = nullptr;

        VkBuffer indexBuffer = nullptr;
        VkDeviceMemory indexBufferMemory = nullptr;

        VkBuffer ssboBuffer = nullptr;
        VkDeviceMemory ssboBufferMemory = nullptr;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        
        VkImage depthImage = nullptr;
        VkDeviceMemory depthImageMemory = nullptr;
        VkImageView depthImageView = nullptr;
        
        VkSampler textureSampler = nullptr;
        
        Models models;
        Components components;
        
        RenderContext context;

        Graphics();
        bool initSDL();
        bool initVulkan(const std::string & appName, uint32_t version);

        void queryVkInstanceExtensions();

        std::vector<VkExtensionProperties> queryPhysicalDeviceExtensions(const VkPhysicalDevice & device);
        bool doesPhysicalDeviceSupportExtension(const VkPhysicalDevice & device, const std::string extension);

        std::vector<VkSurfaceFormatKHR> queryPhysicalDeviceSurfaceFormats(const VkPhysicalDevice & device);
        void listPhysicalDeviceSurfaceFormats(const VkPhysicalDevice & device);
        bool isPhysicalDeviceSurfaceFormatsSupported(const VkPhysicalDevice & device, const VkSurfaceFormatKHR & format);

        std::vector<VkPresentModeKHR> queryDeviceSwapModes();
        VkPresentModeKHR pickBestDeviceSwapMode(const std::vector<VkPresentModeKHR> & availableSwapModes);

        bool createShaderStageInfo();
        
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
        
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        bool createDepthResources();
        bool findDepthFormat(VkFormat & supportedFormat);
        bool findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat & supportedFormat);
        bool createImage(
                int32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        bool transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        void prepareModelTextures();
        void copyBufferToImage(VkBuffer & buffer, VkImage & image, uint32_t width, uint32_t height);
        bool createTextureSampler();
        void copyModelsContentIntoBuffer(void* data, ModelsContentType modelsContentType, VkDeviceSize maxSize);
        void draw(RenderContext & context, int commandBufferIndex, bool useIndices);
        
    public:
        Graphics(const Graphics&) = delete;
        Graphics& operator=(const Graphics &) = delete;
        Graphics(Graphics &&) = delete;
        Graphics & operator=(Graphics) = delete;

        static Graphics & instance();
        bool isActive();
        void init(const std::string & appName, uint32_t version, const std::string & dir);

        void listVkExtensions();
        void listPhysicalDeviceExtensions();
        void listVkLayerNames();
        void listVkPhysicalDevices();

        bool updateSwapChain();
        void drawFrame();
        void addModel(Model * model);
        void addModel(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices, std::string name);
        void addModel(const std::string & dir, const std::string & file);
        bool prepareModels();

        VkExtent2D getWindowExtent();
        
        RenderContext & getRenderContext();
        void renderScene();
        void toggleWireFrame();
        SDL_Window * getSdlWindow();
        
        void prepareComponents();
        Component * addModelComponent(std::string modelLocation);
        
        Models & getModels();
        ModelSummary getModelsBufferSizes();

        ~Graphics();

};

#endif
