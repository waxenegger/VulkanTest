#ifndef SRC_INCLUDES_MODELS_H_
#define SRC_INCLUDES_MODELS_H_

#include "camera.h"

class Vertex final {
    private:
        glm::vec3 position;
        glm::vec3 color;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;
        glm::vec3 bitangent;

    public:
        Vertex(const glm::vec3 & position);
        Vertex(const glm::vec3 & position, const glm::vec3 & color);

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions();

        void setUV(const glm::vec2 & uv);
        void setNormal(const glm::vec3 & normal);
        void setTangent(const glm::vec3 & tangent);
        void setBitangent(const glm::vec3 & bitangent);
        void setColor(const glm::vec3 & color);
};

struct TextureInformation final {
    public:
        int16_t ambientTexture = -1;
        std::string ambientTextureLocation;
        int16_t diffuseTexture = -1;
        std::string diffuseTextureLocation;
        int16_t specularTexture = -1;
        std::string specularTextureLocation;
        int16_t normalTexture = -1;
        std::string normalTextureLocation;
};

class Mesh final {
    private:
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        TextureInformation textures;
    public:
        Mesh(const std::vector<Vertex> & vertices);
        Mesh(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices);
        Mesh(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices, const TextureInformation & textures);
        const std::vector<Vertex> & getVertices() const;
        const std::vector<uint32_t> & getIndices() const;
        void setColor(glm::vec3 color);
        TextureInformation getTextureInformation();
        void setTextureInformation(TextureInformation & TextureInformation);
};

class Texture final {
    private:
        unsigned int id = 0;
        std::string type;
        std::string path;
        bool loaded = false;
        bool valid = false;
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        SDL_Surface * textureSurface = nullptr;
        VkImage textureImage = nullptr;
        VkDeviceMemory textureImageMemory = nullptr;
        VkImageView textureImageView = nullptr;
        
    public:
        unsigned int getId();
        std::string getType();
        bool isValid();
        VkFormat getImageFormat();
        void setId(const unsigned int & id);
        void setType(const std::string & type);
        void setPath(const std::string & path);
        void load();
        uint32_t getWidth();
        uint32_t getHeight();
        VkDeviceSize getSize();
        void * getPixels();
        void freeSurface();
        Texture() {};
        ~Texture();
        void cleanUpTexture(const VkDevice & device);
        bool readImageFormat();
        void setTextureImage(VkImage & image);
        void setTextureImageMemory(VkDeviceMemory & imageMemory);
        void setTextureImageView(VkImageView & imageView);
};

struct RenderContext {
    std::vector<VkCommandBuffer> commandBuffers;
    VkPipelineLayout graphicsPipelineLayout = nullptr;
};


class Model final {
    private:
        std::string file;
        std::string dir;
        std::vector<Mesh> meshes;
        bool loaded = false;
        bool visible = true;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        float scaleFactor = 1.0f;

        void processNode(const aiNode * node, const aiScene *scene);
        Mesh processMesh(const aiMesh *mesh, const aiScene *scene);
    public:
        ~Model();
        Model() {};
        Model(const std::string & dir, const std::string & file);
        Model(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices);
        void init();
        bool hasBeenLoaded();
        bool isVisible();
        std::string getPath();
        std::vector<Mesh> & getMeshes();
        void setColor(glm::vec3 color);
        void setPosition(float x, float y, float z);
        void setPosition(glm::vec3 position);
        void setRotation(int xAxis = 0, int yAxis = 0, int zAxis = 0);
        void scale(float factor);
        glm::mat4 getModelMatrix();
        TextureInformation addTextures(const aiMaterial * mat);
        void correctTexturePath(char * path);
    
};

class Models final {
    private:
        std::map<std::string, std::unique_ptr<Texture>> textures;
        std::vector<std::unique_ptr<Model>> models;
        VkDeviceSize totalNumberOfVertices = 0;
        VkDeviceSize totalNumberOfIndices = 0;

    public:
        void addModel(Model * model);
        void clear();
        void draw(RenderContext & context, int commandBufferIndex, bool useIndices);
        void copyModelsContentIntoBuffer(void* data, bool contentIsIndices, VkDeviceSize maxSize);
        VkDeviceSize getTotalNumberOfVertices();
        VkDeviceSize getTotalNumberOfIndices();
        void setColor(glm::vec3 color);
        void setPosition(float x, float y, float z);
        // TODO: more translation and rotation methods
        const static std::string AMBIENT_TEXTURE;
        const static std::string DIFFUSE_TEXTURE;
        const static std::string SPECULAR_TEXTURE;
        const static std::string TEXTURE_NORMALS;
        void processTextures(Mesh & mesh);
        std::map<std::string, std::unique_ptr<Texture>> &  getTextures();
        void cleanUpTextures(const VkDevice & device);
        ~Models();

};

#endif
