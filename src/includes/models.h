#ifndef SRC_INCLUDES_MODELS_H_
#define SRC_INCLUDES_MODELS_H_

#include "shared.h"

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
};

class Model final {
    private:
        std::string file;
        std::string dir;
        std::vector<Mesh> meshes;
        bool loaded = false;

        void processNode(const aiNode * node, const aiScene *scene);
        Mesh processMesh(const aiMesh *mesh, const aiScene *scene);
    public:
        ~Model() {}
        Model() {};
        Model(const std::string & dir, const std::string & file);
        Model(const std::vector<Mesh> meshes);
        void init();
        bool hasBeenLoaded() {
            return this->loaded;
        };
        std::string getPath() {
            return this->file;
        }
        std::vector<Mesh> & getMeshes();
};

class Models final {
    private:
        std::vector<Vertex> totalOfVertices;
        std::vector<VkDeviceSize> vertexOffsets;
        std::vector<uint32_t> totalOfIndices;
        std::vector<VkDeviceSize> indexOffsets;

    public:
        void addModel(Model & model);
        void clear();
        std::vector<Vertex> & getTotalVertices();
        std::vector<VkDeviceSize> & getVertexOffsets();
        std::vector<uint32_t> & getTotalIndices();
        std::vector<VkDeviceSize> & getIndexOffsets();
};

#endif
