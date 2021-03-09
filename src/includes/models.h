#ifndef SRC_INCLUDES_MODELS_H_
#define SRC_INCLUDES_MODELS_H_

#include "camera.h"

struct MeshProperties final {
    public:
        int ambientTexture = -1;
        int diffuseTexture = -1;
        int specularTexture = -1;
        int normalTexture = -1;
};

struct ModelProperties final {
    public:
        glm::mat4 matrix = glm::mat4(1);
};

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
        static std::array<VkVertexInputAttributeDescription, 6> getAttributeDescriptions();

        void setUV(const glm::vec2 & uv);
        void setNormal(const glm::vec3 & normal);
        void setTangent(const glm::vec3 & tangent);
        void setBitangent(const glm::vec3 & bitangent);
        void setColor(const glm::vec3 & color);
        glm::vec3 getPosition();
};

struct TextureInformation final {
    public:
        int ambientTexture = -1;
        std::string ambientTextureLocation;
        int diffuseTexture = -1;
        std::string diffuseTextureLocation;
        int specularTexture = -1;
        std::string specularTextureLocation;
        int normalTexture = -1;
        std::string normalTextureLocation;
        bool hasTextures() {
          return this->ambientTexture != -1 || this->diffuseTexture != -1 ||
                this->specularTexture != -1 || this->normalTexture != -1;   
        }
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
        Texture(bool empty = false, VkExtent2D extent = {100, 100});
        ~Texture();
        void cleanUpTexture(const VkDevice & device);
        bool readImageFormat();
        void setTextureImage(VkImage & image);
        void setTextureImageMemory(VkDeviceMemory & imageMemory);
        void setTextureImageView(VkImageView & imageView);
        VkImageView & getTextureImageView();
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
        VkDeviceSize ssboOffset = 0;


        void processNode(const aiNode * node, const aiScene *scene);
        Mesh processMesh(const aiMesh *mesh, const aiScene *scene);
        void calculateNormals();
    public:
        ~Model();
        Model() {};
        Model(const std::string & dir, const std::string & file);
        Model(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices, std::string name);
        void init();
        bool hasBeenLoaded();
        bool isVisible();
        std::string getPath();
        std::vector<Mesh> & getMeshes();
        void setColor(glm::vec3 color);
        TextureInformation addTextures(const aiMaterial * mat);
        void correctTexturePath(char * path);
        void setSsboOffset(VkDeviceSize offset);
        VkDeviceSize getSsboOffset();
};

enum ModelsContentType {
    VERTEX, INDEX, SSBO
};

class Models final {
    private:
        std::map<std::string, std::unique_ptr<Texture>> textures;
        std::vector<std::unique_ptr<Model>> models;

    public:
        void addModel(Model * model);
        void clear();
        void setColor(glm::vec3 color);
        const static std::string AMBIENT_TEXTURE;
        const static std::string DIFFUSE_TEXTURE;
        const static std::string SPECULAR_TEXTURE;
        const static std::string TEXTURE_NORMALS;
        void processTextures(Mesh & mesh);
        std::map<std::string, std::unique_ptr<Texture>> &  getTextures();
        std::vector<std::string> getModelLocations();
        VkImageView findTextureImageViewById(unsigned int id); 
        void cleanUpTextures(const VkDevice & device);
        std::vector<std::unique_ptr<Model>> & getModels();
        Model * findModelByLocation(std::string path);
        ~Models();

};

#endif
