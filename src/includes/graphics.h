#ifndef SRC_INCLUDES_GRAPHICS_H_
#define SRC_INCLUDES_GRAPHICS_H_

#include "components.h"
#include "utils.h"

const int MAX_TEXTURES = 25;

const std::vector<Vertex> SKYBOX_VERTICES = {
    Vertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
    Vertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
    Vertex(glm::vec3(1.0f, -1.0f, -1.0f)),
    Vertex(glm::vec3(1.0f, -1.0f, -1.0f)),
    Vertex(glm::vec3(1.0f,  1.0f, -1.0f)),
    Vertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
    
    Vertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
    Vertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
    Vertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
    Vertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
    Vertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
    Vertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
    
    Vertex(glm::vec3(1.0f, -1.0f, -1.0f)),
    Vertex(glm::vec3(1.0f, -1.0f,  1.0f)),
    Vertex(glm::vec3(1.0f,  1.0f,  1.0f)),
    Vertex(glm::vec3(1.0f,  1.0f,  1.0f)),
    Vertex(glm::vec3(1.0f,  1.0f, -1.0f)),
    Vertex(glm::vec3(1.0f, -1.0f, -1.0f)),

    Vertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
    Vertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
    Vertex(glm::vec3(1.0f,  1.0f,  1.0f)),
    Vertex(glm::vec3(1.0f,  1.0f,  1.0f)),
    Vertex(glm::vec3(1.0f, -1.0f,  1.0f)),
    Vertex(glm::vec3(-1.0f, -1.0f,  1.0f)),

    Vertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
    Vertex(glm::vec3(1.0f,  1.0f, -1.0f)),
    Vertex(glm::vec3(1.0f,  1.0f,  1.0f)),
    Vertex(glm::vec3(1.0f,  1.0f,  1.0f)),
    Vertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
    Vertex(glm::vec3(-1.0f,  1.0f, -1.0f)),

    Vertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
    Vertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
    Vertex(glm::vec3(1.0f, -1.0f, -1.0f)),
    Vertex(glm::vec3(1.0f, -1.0f, -1.0f)),
    Vertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
    Vertex(glm::vec3(1.0f, -1.0f,  1.0f))
};

struct ModelSummary {
    VkDeviceSize vertexBufferSize = 0;
    VkDeviceSize indexBufferSize = 0;
    VkDeviceSize meshSsboBufferSize = 0;
    VkDeviceSize modelBufferSize = 0;
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
           //"VK_LAYER_KHRONOS_validation"
        };

        bool showWireFrame = false;
        bool requiresUpdateSwapChain = false;
        
        uint16_t frameCount = 0;
        double deltaTime = 1;
        std::chrono::high_resolution_clock::time_point lastTimeMeasure = std::chrono::high_resolution_clock::now();
        
        VkPhysicalDevice physicalDevice = nullptr;
        VkDevice device = nullptr;

        bool hasSkybox = false;
        
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
        VkDescriptorPool skyboxDescriptorPool = nullptr;

        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<VkDescriptorSet> skyboxDescriptorSets;
        
        VkRenderPass renderPass = nullptr;
        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorSetLayout skyboxDescriptorSetLayout;
        
        VkPipeline graphicsPipeline = nullptr;
        std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageInfo;
        
        VkPipeline skyboxGraphicsPipeline = nullptr;
        VkPipelineLayout skyboxGraphicsPipelineLayout = nullptr;
        std::array<VkPipelineShaderStageCreateInfo, 2> skyboxShaderStageInfo;
        
        std::vector<VkFramebuffer> swapChainFramebuffers;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;

        size_t currentFrame = 0;

        VkBuffer vertexBuffer = nullptr;
        VkDeviceMemory vertexBufferMemory = nullptr;
        
        VkBuffer skyBoxVertexBuffer = nullptr;
        VkDeviceMemory skyBoxVertexBufferMemory = nullptr;        
        
        VkImage skyboxCubeImage = nullptr;
        VkDeviceMemory skyboxCubeImageMemory = nullptr;
        VkImageView skyboxImageView = nullptr;
        VkShaderModule skyboxVertShaderModule = nullptr;
        VkShaderModule skyboxFragShaderModule = nullptr;
        VkShaderModule vertShaderModule = nullptr;
        VkShaderModule fragShaderModule = nullptr;

        VkBuffer indexBuffer = nullptr;
        VkDeviceMemory indexBufferMemory = nullptr;

        VkBuffer meshSsboBuffer = nullptr;
        VkDeviceMemory meshSsboBufferMemory = nullptr;

        VkBuffer modelVertexBuffer = nullptr;
        VkDeviceMemory modelVertexBufferMemory = nullptr;

        VkBuffer indirectCommandBuffer = nullptr;
        VkDeviceMemory indirectCommandBufferMemory = nullptr;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        
        std::vector<VkImage> depthImages;
        std::vector<VkDeviceMemory> depthImagesMemory;
        std::vector<VkImageView> depthImagesView;
        
        VkSampler textureSampler = nullptr;
        VkSampler skyboxSampler = nullptr;
        
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
        bool createSkyboxShaderStageInfo();
        bool createSkybox();
        
        bool createImageViews();

        void createVkInstance(const std::string & appName, uint32_t version);

        bool createGraphicsPipeline();
        bool createSkyboxGraphicsPipeline();
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
        bool createSkyboxDescriptorPool();
        bool createDescriptorSetLayout();
        bool createSkyboxDescriptorSetLayout();
        bool createDescriptorSets();
        bool createSkyboxDescriptorSets();
        
        bool createUniformBuffers();
        void updateUniformBuffer(uint32_t currentImage);
        void updateModelVertexBuffer();

        void listVkPhysicalDeviceQueueFamilyProperties(const VkPhysicalDevice & device);

        bool createCommandPool();
        bool createFramebuffers();
        bool createCommandBuffers();
        bool createCommandBuffer(uint16_t commandBufferIndex);
        bool createRenderPass();

        bool createSyncObjects();

        void cleanupSwapChain();

        bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        bool findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, uint32_t & memoryType);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        bool createBuffersFromModel();
        bool createSsboBufferFromModel(VkDeviceSize bufferSize, bool makeHostWritable = false);
        
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t layerCount = 1);
        bool createDepthResources();
        bool findDepthFormat(VkFormat & supportedFormat);
        bool findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkFormat & supportedFormat);
        bool createImage(
                int32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint16_t arrayLayers = 1);
        
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        bool transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint16_t layerCount = 1);
        void prepareModelTextures();
        void copyBufferToImage(VkBuffer & buffer, VkImage & image, uint32_t width, uint32_t height, uint16_t layerCount = 1);
        bool createTextureSampler(VkSampler & sampler, VkSamplerAddressMode addressMode);
        void copyModelsContentIntoBuffer(void* data, ModelsContentType modelsContentType, VkDeviceSize maxSize);
        void draw(RenderContext & context, int commandBufferIndex, bool useIndices);
        void drawSkybox(RenderContext & context, int commandBufferIndex);
        
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
        void updateScene(uint16_t commandBufferIndex, bool waitForFences = false);
        void toggleWireFrame();
        SDL_Window * getSdlWindow();
        
        void prepareComponents();
        Component * addModelComponent(std::string modelLocation);
        Components & getComponents();
        
        Models & getModels();
        ModelSummary getModelsBufferSizes();
        
        double getDeltaTime();
        void setDeltaTime(double deltaTime);
        void setLastTimeMeasure(std::chrono::high_resolution_clock::time_point time);

        ~Graphics();

};

#endif
