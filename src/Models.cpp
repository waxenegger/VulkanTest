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

std::array<VkVertexInputAttributeDescription, 4> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, normal);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(Vertex, uv);

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

Mesh::Mesh(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices, const TextureInformation & textures) : Mesh(vertices, indices) {
    this->textures = textures;
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

TextureInformation Mesh::getTextureInformation() {
    return this->textures;
}

void Mesh::setTextureInformation(TextureInformation & textures) {
    this->textures = textures;
}

Model::Model(const std::vector< Vertex >& vertices, const std::vector< uint32_t > indices)
{
    this->file = "";
    this->dir = "";
    this->meshes = { Mesh(vertices, indices) };
    this->loaded = true;
}

Model::Model(const std::string & dir, const std::string & file) {
    this->file = std::string(dir + file);
    this->dir = dir;
    Assimp::Importer importer;

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    const aiScene *scene = importer.ReadFile(this->file.c_str(),
            aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "assimp file read: " << time_span.count() <<  std::endl;

    if (scene == nullptr) {
        std::cerr << importer.GetErrorString() << std::endl;
        return;
    }

    if (scene->HasMeshes()) {
        start = std::chrono::high_resolution_clock::now();

        this->processNode(scene->mRootNode, scene);
        this->loaded = true;

        std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
        std::cout << "assimp mesh process: " << time_span.count() <<  std::endl;
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
     TextureInformation textures;

     if (scene->HasMaterials()) {
         const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

         textures = this->addTextures(material);
     }
     
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

     return Mesh(vertices, indices, textures);
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

glm::vec3 Model::getPosition() {
    return this->position;
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

void Models::addModel(Model * model) {
    if (!model->hasBeenLoaded()) return;

    for (Mesh & m : model->getMeshes()) {
        this->totalNumberOfVertices += m.getVertices().size();
        this->totalNumberOfIndices += m.getIndices().size();
        this->processTextures(m);
    }
    
    this->models.push_back(std::unique_ptr<Model>(model));    
}

VkImageView Models::findTextureImageViewById(unsigned int id) {
    for (auto & t : this->textures) {
        if (t.second->getId() == id) return t.second->getTextureImageView();
    }
    
    return nullptr;
}

void Models::processTextures(Mesh & mesh) {
    TextureInformation textureInfo = mesh.getTextureInformation();
    
    std::map<std::string, std::unique_ptr<Texture>>::iterator val;
    
    if (!textureInfo.ambientTextureLocation.empty()) {
        val = this->textures.find(textureInfo.ambientTextureLocation);
        if (val == this->textures.end()) {
            std::unique_ptr<Texture> texture = std::make_unique<Texture>();
            texture->setPath(textureInfo.ambientTextureLocation);
            texture->load();
            if (texture->isValid()) {
                textureInfo.ambientTexture = static_cast<int>(this->textures.empty() ? 0 : this->textures.size());
                texture->setId(textureInfo.ambientTexture);
                this->textures[textureInfo.ambientTextureLocation] = std::move(texture);
            }
        }
    }

    if (!textureInfo.diffuseTextureLocation.empty()) {
        val = this->textures.find(textureInfo.diffuseTextureLocation);
        if (val == this->textures.end()) {
            std::unique_ptr<Texture> texture = std::make_unique<Texture>();
            texture->setPath(textureInfo.diffuseTextureLocation);
            texture->load();
            if (texture->isValid()) {
                textureInfo.diffuseTexture = static_cast<int>(this->textures.empty() ? 0 : this->textures.size());
                texture->setId(textureInfo.diffuseTexture);
                this->textures[textureInfo.diffuseTextureLocation] = std::move(texture);
            }
        }
    }

    if (!textureInfo.specularTextureLocation.empty()) {
        val = this->textures.find(textureInfo.specularTextureLocation);
        if (val == this->textures.end()) {
            std::unique_ptr<Texture> texture = std::make_unique<Texture>();
            texture->setPath(textureInfo.specularTextureLocation);
            texture->load();
            if (texture->isValid()) {
                textureInfo.specularTexture = static_cast<int>(this->textures.empty() ? 0 : this->textures.size());
                texture->setId(textureInfo.specularTexture);
                this->textures[textureInfo.specularTextureLocation] = std::move(texture);
            }
        }
    }

    if (!textureInfo.normalTextureLocation.empty()) {
        val = this->textures.find(textureInfo.normalTextureLocation);
        if (val == this->textures.end()) {
            std::unique_ptr<Texture> texture = std::make_unique<Texture>();
            texture->setPath(textureInfo.normalTextureLocation);
            texture->load();
            if (texture->isValid()) {
                textureInfo.normalTexture = static_cast<int>(this->textures.empty() ? 0 : this->textures.size());
                texture->setId(textureInfo.normalTexture);
                this->textures[textureInfo.normalTextureLocation] = std::move(texture);
            }
        }
    }
    
    mesh.setTextureInformation(textureInfo);
}

void Models::copyModelsContentIntoBuffer(void* data, bool contentIsIndices, VkDeviceSize maxSize) {
    
    VkDeviceSize overallSize = 0;
    for (auto & model : this->models) {
        for (Mesh & mesh : model->getMeshes()) {            
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
    for (auto & model : this->models) {
        for (Mesh & mesh : model->getMeshes()) {
            VkDeviceSize vertexSize = mesh.getVertices().size();
            VkDeviceSize indexSize = mesh.getIndices().size();

            TextureInformation textureInfo = mesh.getTextureInformation();
            ModelProperties modelProps = { 
                model->getModelMatrix(),
                 textureInfo.ambientTexture,
                 textureInfo.diffuseTexture,
                 textureInfo.specularTexture,
                 textureInfo.normalTexture
            };

            vkCmdPushConstants(
                context.commandBuffers[commandBufferIndex], context.graphicsPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                sizeof(struct ModelProperties), &modelProps);
            
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

TextureInformation Model::addTextures(const aiMaterial * mat) {
    TextureInformation textureInfo;
    
    if (mat->GetTextureCount(aiTextureType_AMBIENT) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_AMBIENT, 0, &str);
        
        if (str.length > 0) this->correctTexturePath(str.data);
        textureInfo.ambientTextureLocation = std::string(this->dir + std::string(str.C_Str()));
    }

    if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);

        if (str.length > 0) this->correctTexturePath(str.data);
        textureInfo.diffuseTextureLocation = std::string(this->dir + std::string(str.C_Str()));
    }

    if (mat->GetTextureCount(aiTextureType_SPECULAR) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_SPECULAR, 0, &str);

        if (str.length > 0) this->correctTexturePath(str.data);
        textureInfo.specularTextureLocation = std::string(this->dir + std::string(str.C_Str()));
    }

    if (mat->GetTextureCount(aiTextureType_NORMALS) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_NORMALS, 0, &str);

        if (str.length > 0) this->correctTexturePath(str.data);
        textureInfo.normalTextureLocation = std::string(this->dir + std::string(str.C_Str()));
    }

    return textureInfo;
}

void Model::correctTexturePath(char * path) {
    int index = 0;

    while(path[index] == '\0') index++;

    if(index != 0) {
        int i = 0;
        while(path[i + index] != '\0') {
            path[i] = path[i + index];
            i++;
        }
        path[i] = '\0';
    }
}

Model::~Model() {
    
}

void Models::clear() {
    this->totalNumberOfVertices = 0;
    this->totalNumberOfIndices = 0;
    this->models.clear();
    this->textures.clear();
}

std::map< std::string, std::unique_ptr< Texture >>& Models::getTextures()
{
    return this->textures;
}

VkDeviceSize Models::getTotalNumberOfVertices() {
    return this->totalNumberOfVertices;
}

VkDeviceSize Models::getTotalNumberOfIndices() {
    return this->totalNumberOfIndices;
}

void Models::setColor(glm::vec3 color) {
    for (auto & model : this->models) {
        model->setColor(color);
    }
}

void Models::setPosition(float x, float y, float z) {
    for (auto & model : this->models) {
        model->setPosition(x,y,z);
    }    
}

void Models::cleanUpTextures(const VkDevice & device) {
    for (auto & texture : this->textures) {
        texture.second->cleanUpTexture(device);
    }
}

Models::~Models() {
    this->clear();
}

unsigned int Texture::getId() {
    return  this->id;
}

std::string Texture::getType() {
    return this->type;
}

bool Texture::isValid() {
    return this->valid;
}

VkFormat Texture::getImageFormat() {
    return this->imageFormat;
}

VkImageView & Texture::getTextureImageView() {
    return this->textureImageView;
}

void Texture::setId(const unsigned int & id) {
    this->id = id;
}

void Texture::setType(const std::string & type) {
    this->type = type;
}

void Texture::setPath(const std::string & path) {
    this->path = path;
}

void Texture::setTextureImage(VkImage & image) {
    this->textureImage = image;
}

void Texture::setTextureImageMemory(VkDeviceMemory & imageMemory) {
    this->textureImageMemory = imageMemory;
}

void Texture::setTextureImageView(VkImageView & imageView) {
    this->textureImageView = imageView;
}

void Texture::load() {
    if (!this->loaded) {
        std::cerr << "Loading texture: " << this->path << std::endl;
        this->textureSurface = IMG_Load(this->path.c_str());
        if (this->textureSurface != nullptr) {
            if (!this->readImageFormat()) {
                std::cerr << "Unsupported Texture Format: " << this->path << std::endl;
            } else if (this->getSize() != 0) {
                this->valid = true;
            }
        } else std::cerr << "Failed to load texture: " << this->path << std::endl;
        this->loaded = true;
    }
}

void Texture::cleanUpTexture(const VkDevice & device) {
    if (device == nullptr) return;
    
    if (this->textureImage != nullptr) {
        vkDestroyImage(device, this->textureImage, nullptr);        
        this->textureImage = nullptr;
    }

    if (this->textureImageMemory != nullptr) {
        vkFreeMemory(device, this->textureImageMemory, nullptr);
        this->textureImageMemory = nullptr;
    }

    if (this->textureImageView != nullptr) {
        vkDestroyImageView(device, this->textureImageView, nullptr);
        this->textureImageView = nullptr;
    }
}

uint32_t Texture::getWidth() {
    return this->textureSurface == nullptr ? 0 : this->textureSurface->w; 
}

uint32_t Texture::getHeight() {
    return this->textureSurface == nullptr ? 0 : this->textureSurface->h;     
}

VkDeviceSize Texture::getSize() {
    int channels = this->textureSurface == nullptr ? 0 : textureSurface->format->BytesPerPixel;
    return this->getWidth() * this->getHeight() * channels;
}

void * Texture::getPixels() {
 return this->textureSurface == nullptr ? nullptr : this->textureSurface->pixels;
}

void Texture::freeSurface() {
    if (this->textureSurface != nullptr) {
        SDL_FreeSurface(this->textureSurface);
        this->textureSurface = nullptr;
    }    
}

Texture::~Texture() {
    this->freeSurface();
}

bool Texture::readImageFormat() {
    if (this->textureSurface == nullptr) return false;

    const int nOfColors = this->textureSurface->format->BytesPerPixel;

    if (nOfColors == 4) {
        if (this->textureSurface->format->Rmask == 0x000000ff) this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        else this->imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    } else if (nOfColors == 3) {
        Uint32 aMask = 0x000000ff;
        if (this->textureSurface->format->Rmask == 0x000000ff) this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        else {
            this->imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
            aMask = 0xff000000;
        }
                
        // convert to 32 bit
        SDL_Surface * tmpSurface = SDL_CreateRGBSurface(
            0, this->textureSurface->w, this->textureSurface->h, 32, 
            this->textureSurface->format->Rmask, this->textureSurface->format->Gmask, this->textureSurface->format->Bmask, aMask);

        // attempt twice with different pixel format
        if (tmpSurface == nullptr) {
            std::cerr << "Conversion Failed. Try something else for: " << this->path << std::endl;

            tmpSurface = SDL_CreateRGBSurface(
                0, this->textureSurface->w, this->textureSurface->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);                

            if (tmpSurface == nullptr) {
                tmpSurface = SDL_CreateRGBSurface(
                    0, this->textureSurface->w, this->textureSurface->h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);                
            }
        }
        
        // either conversion has worked or not
        if (tmpSurface != nullptr) {            
            SDL_SetSurfaceAlphaMod(tmpSurface, 0);
            if (SDL_BlitSurface(this->textureSurface, nullptr, tmpSurface , nullptr) == 0) {
                SDL_FreeSurface(this->textureSurface);
                this->textureSurface = tmpSurface;
            } else {
                std::cerr << "SDL_BlitSurface Failed (on conversion): " << SDL_GetError() << std::endl;
                return false;
            }
        } else {
            std::cerr << "SDL_CreateRGBSurface Failed (on conversion): " << SDL_GetError() << std::endl;
            return false;
        }
    } else return false;

    return true;
}

const std::string Models::AMBIENT_TEXTURE = "ambient";
const std::string Models::DIFFUSE_TEXTURE = "diffuse";
const std::string Models::SPECULAR_TEXTURE = "specular";
const std::string Models::TEXTURE_NORMALS = "normals";
