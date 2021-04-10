#include <src/includes/models.h>

SimpleVertex::SimpleVertex(const glm::vec3 & position) {
    this->position = position;
}

ColorVertex::ColorVertex(const glm::vec3 & position) {
    this->position = position;
}

void ColorVertex::setColor(const glm::vec3 & color) {
    this->color = color;
}

ModelVertex::ModelVertex(const glm::vec3 & position) {
    this->position = position;
}

VkVertexInputBindingDescription ModelVertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(ModelVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 5> ModelVertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(ModelVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(ModelVertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(ModelVertex, uv);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(ModelVertex, tangent);

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(ModelVertex, bitangent);

    return attributeDescriptions;
}

VkVertexInputBindingDescription SimpleVertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(SimpleVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 1> SimpleVertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(SimpleVertex, position);

    return attributeDescriptions;
}

VkVertexInputBindingDescription ColorVertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(ColorVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> ColorVertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(ColorVertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(ColorVertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(ColorVertex, uv);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(ColorVertex, color);

    return attributeDescriptions;
}

void ColorVertex::setUV(const glm::vec2 & uv) {
    this->uv = uv;
}

void ColorVertex::setNormal(const glm::vec3 & normal) {
    this->normal = normal;
}

glm::vec3 ColorVertex::getPosition() {
    return this->position;
}

glm::vec3 ModelVertex::getPosition() {
    return this->position;
}

void ModelVertex::setUV(const glm::vec2 & uv) {
    this->uv = uv;
}

void ModelVertex::setNormal(const glm::vec3 & normal) {
    this->normal = normal;
}

void ModelVertex::setTangent(const glm::vec3 & tangent) {
    this->tangent = tangent;
}

void ModelVertex::setBitangent(const glm::vec3 & bitangent) {
    this->bitangent = bitangent;
}

glm::vec3 SimpleVertex::getPosition() {
    return this->position;
}

Mesh::Mesh(const std::vector<ModelVertex> & vertices) {
    this->vertices = vertices;
}

Mesh::Mesh(const std::vector<ModelVertex> & vertices, const std::vector<uint32_t> indices) : Mesh(vertices) {
    this->indices = indices;
}

Mesh::Mesh(const std::vector<ModelVertex> & vertices, const std::vector<uint32_t> indices, 
           const TextureInformation & textures, const MaterialInformation & materials) : Mesh(vertices, indices) {
    this->textures = textures;
    this->materials = materials;
}

const std::vector<ModelVertex> & Mesh::getVertices() const {
    return this->vertices;
}

const std::vector<uint32_t> & Mesh::getIndices() const {
    return this->indices;
}

void Mesh::setColor(glm::vec4 color) {
    this->materials.diffuseColor = color;
}

void Mesh::setOpacity(float opacity) {
    this->materials.opacity = opacity;
}

std::string Mesh::getName() {
    return this->name;
}

void Mesh::setName(std::string name) {
    this->name = name;
}

TextureInformation Mesh::getTextureInformation() {
    return this->textures;
}

MaterialInformation Mesh::getMaterialInformation() {
    return this->materials;
}

void Mesh::setTextureInformation(TextureInformation & textures) {
    this->textures = textures;
}

void Mesh::setBoundingBox(BoundingBox & bbox) {
    this->bbox = bbox;
}

BoundingBox & Mesh::getBoundingBox() {
    return this->bbox;
}

bool Mesh::isBoundingBox() {
    return this->isBbox;
}

void Mesh::markAsBoundingBox() {
    this->isBbox = true;
}


