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

void Vertex::setColor(const glm::vec3 & color) {
    this->color = color;
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

void Mesh::setColor(glm::vec3 color) {
    for (Vertex & v : this->vertices) {
        v.setColor(color);
    }   
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

bool Model::hasBeenLoaded() {
    return this->loaded;
};

bool Model::isVisible() {
    return this->visible;
};

std::string Model::getPath() {
    return this->file;
}


Mesh Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
     std::vector<Vertex> vertices;
     std::vector<unsigned int> indices;

     // TODO
     if (scene->HasMaterials()) {}

     if (mesh->mNumVertices > 0) vertices.reserve(mesh->mNumVertices);
     for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
         Vertex vertex(glm::vec3(mesh->mVertices[i].x, -mesh->mVertices[i].y, mesh->mVertices[i].z), glm::vec3(1.0f));

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

glm::mat4 Model::getModelMatrix() {
    glm::mat4 transformation = glm::mat4(1.0f);

    transformation = glm::translate(transformation, this->position);

    if (this->rotation.x != 0.0f) transformation = glm::rotate(transformation, this->rotation.x, glm::vec3(1, 0, 0));
    if (this->rotation.y != 0.0f) transformation = glm::rotate(transformation, this->rotation.y, glm::vec3(0, 1, 0));
    if (this->rotation.z != 0.0f) transformation = glm::rotate(transformation, this->rotation.z, glm::vec3(0, 0, 1));

    return glm::scale(transformation, glm::vec3(this->scaleFactor));   
}

void Model::setColor(glm::vec3 color) {
    for (Mesh & m : this->meshes) {
        m.setColor(color);
    }
}

void Model::setPosition(float x, float y, float z) {
    this->setPosition(glm::vec3(x,y,z));
}

void Model::setPosition(glm::vec3 position) {
    this->position = position;
}

void Model::setRotation(int xAxis, int yAxis, int zAxis) {
    this->rotation.x = glm::radians(static_cast<float>(xAxis));
    this->rotation.y = glm::radians(static_cast<float>(yAxis));
    this->rotation.z = glm::radians(static_cast<float>(zAxis));
}

void Model::scale(float factor) {
    if (factor <= 0) return;
    
    this->scaleFactor = factor;
}

void Models::addModel(Model & model) {
    if (!model.hasBeenLoaded()) return;
    
    this->models.push_back(std::move(model));
    
    for (Mesh & m : model.getMeshes()) {
        this->totalNumberOfVertices += m.getVertices().size();
        this->totalNumberOfIndices += m.getIndices().size();
    }
}

void Models::copyModelsContentIntoBuffer(void* data, bool contentIsIndices, VkDeviceSize maxSize) {
    
    VkDeviceSize overallSize = 0;
    for (Model & model : this->models) {
        for (Mesh & mesh : model.getMeshes()) {
            VkDeviceSize dataSize = 0;
            if (contentIsIndices) {
                dataSize = mesh.getIndices().size() * sizeof(uint32_t);
                if (overallSize + dataSize <= maxSize) {
                    memcpy(static_cast<char *>(data) + overallSize, mesh.getIndices().data(), dataSize);
                }
            } else {
                dataSize = mesh.getVertices().size() * sizeof(class Vertex);
                if (overallSize + dataSize <= maxSize) {
                    memcpy(static_cast<char *>(data)+overallSize, mesh.getVertices().data(), dataSize);
                }
            }
            overallSize += dataSize;
        }
    }
    
}

void Models::draw(RenderContext & context, int commandBufferIndex, bool useIndices) {
    
    VkDeviceSize lastVertexOffset = 0;
    VkDeviceSize lastIndexOffset = 0;

    int c = 0;
    for (Model & model : this->models) {

        std::vector<ModelProperties> modelProperties;
        modelProperties.push_back(ModelProperties { model.getModelMatrix() });

        vkCmdPushConstants(
            context.commandBuffers[commandBufferIndex], context.graphicsPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT, 0,
            sizeof(struct ModelProperties), modelProperties.data());
        
        for (Mesh & mesh : model.getMeshes()) {
            VkDeviceSize vertexSize = mesh.getVertices().size();
            VkDeviceSize indexSize = mesh.getIndices().size();
            
            if (useIndices) {
                vkCmdDrawIndexed(context.commandBuffers[commandBufferIndex], indexSize , 1, lastIndexOffset, lastVertexOffset, 0);
            } else {
                vkCmdDraw(context.commandBuffers[commandBufferIndex], vertexSize, 1, 0, 0);
            }
                        
            lastIndexOffset += indexSize;
            lastVertexOffset += vertexSize;
        }

        c++;
    }
}

void Models::clear() {
    this->totalNumberOfVertices = 0;
    this->totalNumberOfIndices = 0;
    this->models.clear();
}

VkDeviceSize Models::getTotalNumberOfVertices() {
    return this->totalNumberOfVertices;
}

VkDeviceSize Models::getTotalNumberOfIndices() {
    return this->totalNumberOfIndices;
}

void Models::setColor(glm::vec3 color) {
    for (Model & model : this->models) {
        model.setColor(color);
    }
}

void Models::setPosition(float x, float y, float z) {
    for (Model & model : this->models) {
        model.setPosition(x,y,z);
    }    
}
