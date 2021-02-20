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

void Vertex::setUV(const glm::vec2 & uv) {
    this->uv = uv;
}


void Vertex::setNormal(const glm::vec3 & normal) {
    this->normal = normal;
}

void Vertex::setTangent(const glm::vec3 & tangent) {
    this->tangent = tangent;
}

void Vertex::setBitangent(const glm::vec3 & bitangent) {
    this->bitangent = bitangent;
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

Model::Model(const std::vector<Mesh> meshes) {
    this->file = "";
    this->dir = "";
    this->meshes = meshes;
    this->loaded = true;
}

Model::Model(const std::string & dir, const std::string & file) {
    this->file = std::string(dir + file);
    this->dir = dir;
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(this->file.c_str(),
            aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

    if (scene == nullptr) {
        std::cerr << importer.GetErrorString() << std::endl;
        return;
    }

    if (scene->HasMeshes()) {
        this->processNode(scene->mRootNode, scene);
        this->loaded = true;
    } else std::cerr << "Model does not contain meshes" << std::endl;
}

void Model::processNode(const aiNode * node, const aiScene *scene) {
    for(unsigned int i=0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        this->meshes.push_back(this->processMesh(mesh, scene));
    }

    for(unsigned int i=0; i<node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

std::vector<Mesh> & Model::getMeshes() {
    return this->meshes;
}

Mesh Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
     std::vector<Vertex> vertices;
     std::vector<unsigned int> indices;

     // TODO
     if (scene->HasMaterials()) {}

     if (mesh->mNumVertices > 0) vertices.reserve(mesh->mNumVertices);
     for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
         Vertex vertex(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z), glm::vec3(1.0f));

         if (mesh->HasNormals())
             vertex.setNormal(glm::normalize(glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)));

         if(mesh->HasTextureCoords(0)) {
             vertex.setUV(glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y));
         } else vertex.setUV(glm::vec2(0.0f, 0.0f));

         if (mesh->HasTangentsAndBitangents()) {
             if (mesh->mTangents->Length() == mesh->mNumVertices)
                 vertex.setTangent(glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z));

             if (mesh->mBitangents->Length() == mesh->mNumVertices)
                 vertex.setBitangent(glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z));
         }

         vertices.push_back(vertex);
     }

     for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
         const aiFace face = mesh->mFaces[i];

         for(unsigned int j = 0; j < face.mNumIndices; j++) indices.push_back(face.mIndices[j]);
     }

     return Mesh(vertices, indices);
}

void Models::addModel(Model & model) {
    VkDeviceSize vertexOffsetPerModel = 0;
    VkDeviceSize indexOffsetPerModel = 0;

    for (auto & mesh : model.getMeshes()) {
        auto meshVertices = mesh.getVertices();
        for (auto & vertex : meshVertices) {
            this->totalOfVertices.push_back(vertex);
        }
        if (!meshVertices.empty()) {
            vertexOffsetPerModel += meshVertices.size();
            this->vertexOffsets.push_back(vertexOffsetPerModel);
        }

        auto meshIndices = mesh.getIndices();
        for (auto & index : meshIndices) {
            this->totalOfIndices.push_back(index);
        }
        if (!meshIndices.empty()) {
            indexOffsetPerModel += meshIndices.size();
            this->indexOffsets.push_back(indexOffsetPerModel);
        }
    }
}

std::vector<VkDeviceSize> & Models::getIndexOffsets() {
    return this->indexOffsets;
}

std::vector<VkDeviceSize> & Models::getVertexOffsets() {
    return this->vertexOffsets;
}

void Models::clear() {
    this->totalOfIndices.clear();
    this->totalOfVertices.clear();
    this->indexOffsets.clear();
    this->vertexOffsets.clear();
}

std::vector<Vertex> &  Models::getTotalVertices() {
    return this->totalOfVertices;
}

std::vector<uint32_t> & Models::getTotalIndices() {
    return this->totalOfIndices;
}
