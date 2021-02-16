#include <src/includes/models.h>

Vertex::Vertex(const glm::vec3 & position) {
    this->position = position;
}

Vertex::Vertex(const glm::vec3 & position, const glm::vec3 & color) : Vertex(position) {
    this->color = color;
}

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    return attributeDescriptions;
}

Mesh::Mesh(const std::vector<Vertex> & vertices) {
    this->vertices = vertices;
}

Mesh::Mesh(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices) : Mesh(vertices) {
    this->indices = indices;
}

const std::vector<Vertex> & Mesh::getVertices() const {
    return this->vertices;
}

const std::vector<uint32_t> & Mesh::getIndices() const {
    return this->indices;
}

