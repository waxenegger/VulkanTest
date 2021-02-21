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
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();

        void setUV(const glm::vec2 & uv);
        void setNormal(const glm::vec3 & normal);
        void setTangent(const glm::vec3 & tangent);
        void setBitangent(const glm::vec3 & bitangent);
        void setColor(const glm::vec3 & color);
};

class Mesh final {
    private:
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    public:
        Mesh(const std::vector<Vertex> & vertices);
        Mesh(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices);
        const std::vector<Vertex> & getVertices() const;
        const std::vector<uint32_t> & getIndices() const;
        void setColor(glm::vec3 color);
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
        ~Model() {}
        Model() {};
        Model(const std::string & dir, const std::string & file);
        Model(const std::vector<Mesh> meshes);
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
    
};

class Models final {
    private:
        std::vector<Model> models;
        VkDeviceSize totalNumberOfVertices = 0;
        VkDeviceSize totalNumberOfIndices = 0;

    public:
        void addModel(Model & model);
        void clear();
        void draw(RenderContext & context, int commandBufferIndex, bool useIndices);
        void copyModelsContentIntoBuffer(void* data, bool contentIsIndices, VkDeviceSize maxSize);
        VkDeviceSize getTotalNumberOfVertices();
        VkDeviceSize getTotalNumberOfIndices();
        void setColor(glm::vec3 color);
        void setPosition(float x, float y, float z);
        // TODO: more translation and rotation methods
};

#endif
