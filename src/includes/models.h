#ifndef SRC_INCLUDES_MODELS_H_
#define SRC_INCLUDES_MODELS_H_

#include "shared.h"

class Vertex final {
    private:
        glm::vec3 position;
        glm::vec3 color;

    public:
        Vertex(const glm::vec3 & position);
        Vertex(const glm::vec3 & position, const glm::vec3 & color);

        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
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

class Models final {
};

#endif
