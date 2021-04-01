#ifndef SRC_INCLUDES_MODELS_H_
#define SRC_INCLUDES_MODELS_H_

#include "camera.h"

struct MeshProperties final {
    public:
        int ambientTexture = -1;
        int diffuseTexture = -1;
        int specularTexture = -1;
        int normalTexture = -1;
        glm::vec3 ambientColor = glm::vec3(1.0f);
        float emissiveFactor = 0.1f;
        glm::vec3 diffuseColor = glm::vec3(1.0f);
        float opacity = 1.0f;
        glm::vec3 specularColor = glm::vec3(1.0f);
        float shininess = 1.0f;
};

struct ModelProperties final {
    public:
        glm::mat4 matrix = glm::mat4(1);
};

class Vertex final {
    private:
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
        glm::vec3 tangent;
        glm::vec3 bitangent;

    public:
        Vertex(const glm::vec3 & position);

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions();

        void setUV(const glm::vec2 & uv);
        void setNormal(const glm::vec3 & normal);
        void setTangent(const glm::vec3 & tangent);
        void setBitangent(const glm::vec3 & bitangent);
        glm::vec3 getPosition();
};

struct MaterialInformation final {
    public:
        glm::vec3 ambientColor = glm::vec3(1.0f);
        glm::vec3 diffuseColor = glm::vec3(1.0f);
        glm::vec3 specularColor = glm::vec3(1.0f);
        float emissiveFactor = 0.1f;
        float shininess = 1.0f;
        float opacity = 1.0f;
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
        MaterialInformation materials;
    public:
        Mesh(const std::vector<Vertex> & vertices);
        Mesh(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices);
        Mesh(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices, 
             const TextureInformation & textures, const MaterialInformation & materials);
        const std::vector<Vertex> & getVertices() const;
        const std::vector<uint32_t> & getIndices() const;
        void setColor(glm::vec4 color);
        TextureInformation getTextureInformation();
        MaterialInformation getMaterialInformation();
        void setTextureInformation(TextureInformation & TextureInformation);
};

class Texture final {
    private:
        int id = 0;
        std::string type;
        std::filesystem::path path;
        bool loaded = false;
        bool valid = false;
        VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        SDL_Surface * textureSurface = nullptr;
        VkImage textureImage = nullptr;
        VkDeviceMemory textureImageMemory = nullptr;
        VkImageView textureImageView = nullptr;
        
    public:
        int getId();
        std::string getType();
        bool isValid();
        VkFormat getImageFormat();
        void setId(const int & id);
        void setType(const std::string & type);
        void setPath(const std::filesystem::path & path);
        void load();
        uint32_t getWidth();
        uint32_t getHeight();
        VkDeviceSize getSize();
        void * getPixels();
        void freeSurface();
        Texture(bool empty = false, VkExtent2D extent = {100, 100});
        Texture(SDL_Surface * surface);
        ~Texture();
        void cleanUpTexture(const VkDevice & device);
        bool readImageFormat();
        void setTextureImage(VkImage & image);
        void setTextureImageMemory(VkDeviceMemory & imageMemory);
        void setTextureImageView(VkImageView & imageView);
        VkImageView & getTextureImageView();
};

class Model final {
    private:
        std::string id;
        std::filesystem::path file;
        std::vector<Mesh> meshes;
        bool loaded = false;
        VkDeviceSize ssboOffset = 0;


        void processNode(const aiNode * node, const aiScene *scene);
        Mesh processMesh(const aiMesh *mesh, const aiScene *scene);

        Model(const std::string id);        
    public:
        ~Model();
        Model() {};
        Model(const std::string id, const std::filesystem::path file);
        Model(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices, std::string id);
        void init();
        bool hasBeenLoaded();
        std::filesystem::path getFile();
        std::string getId();
        std::vector<Mesh> & getMeshes();
        void setColor(glm::vec4 color);
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
        void addTextModel(std::string id, std::string font, std::string text, uint16_t size);
        void clear();
        void setColor(glm::vec4 color);
        const static std::string AMBIENT_TEXTURE;
        const static std::string DIFFUSE_TEXTURE;
        const static std::string SPECULAR_TEXTURE;
        const static std::string TEXTURE_NORMALS;
        void processTextures(Mesh & mesh);
        std::map<std::string, std::unique_ptr<Texture>> &  getTextures();
        std::vector<std::string> getModelIds();
        VkImageView findTextureImageViewById(int id); 
        void cleanUpTextures(const VkDevice & device);
        std::vector<std::unique_ptr<Model>> & getModels();
        Model * findModel(std::string id);
        static Model * createPlaneModel(std::string id, VkExtent2D extent);
        ~Models();

};

#endif
