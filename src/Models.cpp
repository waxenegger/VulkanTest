#include <src/includes/models.h>

Vertex::Vertex(const glm::vec3 & position) {
    this->position = position;
}

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 5> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, position);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, uv);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(Vertex, tangent);

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(Vertex, bitangent);

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

glm::vec3 Vertex::getPosition() {
    return this->position;
}

Mesh::Mesh(const std::vector<Vertex> & vertices) {
    this->vertices = vertices;
}

Mesh::Mesh(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices) : Mesh(vertices) {
    this->indices = indices;
}

Mesh::Mesh(const std::vector<Vertex> & vertices, const std::vector<uint32_t> indices, 
           const TextureInformation & textures, const MaterialInformation & materials) : Mesh(vertices, indices) {
    this->textures = textures;
    this->materials = materials;
}

const std::vector<Vertex> & Mesh::getVertices() const {
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

Model::Model(const std::vector< Vertex >& vertices, const std::vector< uint32_t > indices, std::string id) : Model(id)
{
    this->file = "";
    this->meshes = { Mesh(vertices, indices) };
    this->loaded = true;
}

Model::Model(const std::string id, const  std::filesystem::path file) : Model(id) {
    this->file = file;
    Assimp::Importer importer;

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    const aiScene *scene = importer.ReadFile(this->file.c_str(),
            aiProcess_Triangulate | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Read " << this->file << " in: " << time_span.count() <<  std::endl;

    if (scene == nullptr) {
        std::cerr << importer.GetErrorString() << std::endl;
        return;
    }

    if (scene->HasMeshes()) {
        start = std::chrono::high_resolution_clock::now();

        this->processNode(scene->mRootNode, scene);
        
        this->calculateBoundingBoxForModel(DEBUG_BBOX);
                
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

void Model::calculateBoundingBoxForModel(bool addBboxMesh) {
    
    glm::vec3 min = glm::vec3(INF);
    glm::vec3 max = glm::vec3(NEG_INF);
    
    for (auto & m : this->meshes) {
        min.x = std::min(m.getBoundingBox().min.x, min.x);
        min.y = std::min(m.getBoundingBox().min.y, min.y);
        min.z = std::min(m.getBoundingBox().min.z, min.z);
        
        max.x = std::max(m.getBoundingBox().max.x, max.x);
        max.y = std::max(m.getBoundingBox().max.y, max.y);
        max.z = std::max(m.getBoundingBox().max.z, max.z);
    }        
    
    this->bbox.min = min;
    this->bbox.max = max;
    
    if (!addBboxMesh) return;
    
    float padding = 0.01;
    
    std::vector<Vertex> bboxVertices = {
        Vertex(glm::vec3(min.x-padding, min.y-padding, min.z-padding)),            
        Vertex(glm::vec3(min.x-padding, max.y+padding, min.z-padding)),
        Vertex(glm::vec3(max.x+padding, max.y+padding, min.z-padding)),
        Vertex(glm::vec3(max.x+padding, min.y-padding, min.z-padding)),
        Vertex(glm::vec3(min.x-padding, min.y-padding, max.z+padding)),
        Vertex(glm::vec3(min.x-padding, max.y+padding, max.z+padding)),
        Vertex(glm::vec3(max.x+padding, max.y+padding, max.z+padding)),
        Vertex(glm::vec3(max.x+padding, min.y-padding, max.z+padding))
    };
    glm::vec3 edge1 = bboxVertices[3].getPosition() - bboxVertices[1].getPosition();
    glm::vec3 edge2 = bboxVertices[1].getPosition() - bboxVertices[0].getPosition();
    glm::vec cross1 = normalize(glm::cross(edge2, edge1));
    
    glm::vec3 edge3 = bboxVertices[6].getPosition() - bboxVertices[2].getPosition();
    glm::vec3 edge4 = bboxVertices[6].getPosition() - bboxVertices[7].getPosition();
    glm::vec cross2 = normalize(glm::cross(edge4, edge3));

    glm::vec3 edge5 = bboxVertices[5].getPosition() - bboxVertices[1].getPosition();
    glm::vec cross3 = normalize(glm::cross(edge5, edge1));
    
    bboxVertices[0].setNormal((cross1+cross3) / 2.0f);
    bboxVertices[1].setNormal((cross1+cross3) / 2.0f);
    bboxVertices[2].setNormal((cross1+cross2) / 2.0f);
    bboxVertices[3].setNormal((cross1+cross2) / 2.0f);
    bboxVertices[4].setNormal((-cross1+cross3) / 2.0f);
    bboxVertices[5].setNormal((-cross1+cross3) / 2.0f);
    bboxVertices[6].setNormal((-cross1+cross2) / 2.0f);
    bboxVertices[7].setNormal((-cross1+cross2) / 2.0f);
    
    std::vector<uint32_t> bboxIndices = {
        1, 3, 0, 3, 1, 2,
        3, 2, 7, 6, 7, 2,
        1, 0, 4, 4, 5, 1,        
        4, 0, 7 ,7, 0, 3,
        6, 1, 5, 6, 2, 1,        
        5, 4, 7, 6, 5, 7
    };
    
    Mesh mesh = Mesh(bboxVertices, bboxIndices);
    mesh.setColor(glm::vec4(1.0f));
    mesh.setOpacity(0.3);
    this->meshes.push_back(mesh);
}

std::vector<Mesh> & Model::getMeshes() {
    return this->meshes;
}

bool Model::hasBeenLoaded() {
    return this->loaded;
};

std::filesystem::path Model::getFile() {
    return this->file;
}

std::string Model::getId() {
    return this->id;
}

Mesh Model::processMesh(const aiMesh *mesh, const aiScene *scene) {
     std::vector<Vertex> vertices;
     std::vector<unsigned int> indices;
     TextureInformation textures;
     MaterialInformation materials;

     if (scene->HasMaterials()) {
        const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::unique_ptr<aiColor4D> ambient(new aiColor4D());
        std::unique_ptr<aiColor4D> diffuse(new aiColor4D());
        std::unique_ptr<aiColor4D> specular(new aiColor4D());
        
        float opacity = 1.0f;
        aiGetMaterialFloat(material, AI_MATKEY_OPACITY, &opacity);
        if (opacity > 0.0f) materials.opacity = opacity;
        
        const glm::vec3 nullVec3 = glm::vec3(0.0f);
        
        if (aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, ambient.get()) == aiReturn_SUCCESS) {
            glm::vec3 ambientVec3 = glm::vec3(ambient->r, ambient->g, ambient->b);
            if (ambientVec3 != nullVec3) materials.ambientColor = ambientVec3;
        };
        if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, diffuse.get()) == aiReturn_SUCCESS) {
            glm::vec3 diffuseVec3 = glm::vec3(diffuse->r, diffuse->g, diffuse->b);
            if (diffuseVec3 != nullVec3) materials.diffuseColor = diffuseVec3;
        };
        if (aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, specular.get()) == aiReturn_SUCCESS) {
            glm::vec3 specularVec3 = glm::vec3(specular->r, specular->g, specular->b);
            if (specularVec3 != nullVec3) materials.specularColor = specularVec3;
        };

        float shiny = 1.0f;
        aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shiny);
        if (shiny > 0.0f) materials.shininess = shiny;

        textures = this->addTextures(material);
     }
     
     if (mesh->mNumVertices > 0) vertices.reserve(mesh->mNumVertices);
     for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
         Vertex vertex(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z));

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

     Mesh m = Mesh(vertices, indices, textures, materials);
     BoundingBox bbox = {
       glm::vec3(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z),
       glm::vec3(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z)
     };
     m.setBoundingBox(bbox);

     return m;
}

void Model::setColor(glm::vec4 color) {
    for (Mesh & m : this->meshes) {
        m.setColor(color);
    }
}

BoundingBox & Model::getBoundingBox() {
    return this->bbox;
}

void Model::setBoundingBox(BoundingBox bbox) {
    this->bbox = bbox;
}

void Models::addModel(Model* model)
{
    if (!model->hasBeenLoaded()) return;

    auto & meshes = model->getMeshes();
    for (Mesh & m : meshes) {
        this->processTextures(m);
        TextureInformation info =  m.getTextureInformation();
    }
    
    this->models.push_back(std::unique_ptr<Model>(model));    
}

void Models::addTextModel(std::string id, std::string font, std::string text, uint16_t size) {
    TTF_Font * f = TTF_OpenFont(font.c_str(), size);

    if (f != nullptr) {
        TTF_SetFontStyle(f, TTF_STYLE_NORMAL);
        const SDL_Color bg = { 255, 255, 255 };
        const SDL_Color fg = { 0, 0, 0 };

        SDL_Surface * tmp = TTF_RenderUTF8_Shaded(f, text.c_str(), fg, bg);
        
        bool succeeded = false;
        if (tmp != nullptr) {

            SDL_PixelFormat *format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
            std::unique_ptr<Texture> tex = std::make_unique<Texture>(SDL_ConvertSurface(tmp, format, 0));

            SDL_FreeFormat(format);
            SDL_FreeSurface(tmp);
            
            if (tex != nullptr && tex->isValid()) {
                VkExtent2D extent = {
                  tex->getWidth(), tex->getHeight()  
                };
                std::unique_ptr<Model> m(Models::createPlaneModel(id, extent));
                if (m != nullptr && m->hasBeenLoaded()) {
                    
                    tex->setId(this->textures.size());
                    tex->setPath(m->getId());

                    TextureInformation texInfo;
                    texInfo.diffuseTexture = tex->getId();
                    texInfo.diffuseTextureLocation = m->getId();
                    m->getMeshes()[0].setTextureInformation(texInfo);

                    this->textures[m->getId()] = std::move(tex);                    
                    this->addModel(m.release());
                    
                    succeeded = true;
                }
            }
        }

        TTF_CloseFont(f);
        
        if (succeeded) return;
    }
    
    std::cerr << "Failed to add Text Model" << std::endl;
}

Model * Models::createPlaneModel(std::string id, VkExtent2D extent) {
    const float zDirNormal = 1.0f;
    const float w = extent.width / extent.height;
    const float h = 1;

    std::vector<Vertex> vertices;
    
    Vertex one(glm::vec3(-w/2, -h/2, 0.0f));
    one.setUV(glm::vec2(-1.0f, 1.0f));
    one.setNormal(glm::vec3(-1, -1, zDirNormal));
    vertices.push_back(one);

    Vertex two(glm::vec3(-w/2, h/2, 0.0f));
    two.setUV(glm::vec2(-1.0f, 0.0f));
    two.setNormal(glm::vec3(-1, 1, zDirNormal));
    vertices.push_back(two);

    Vertex three(glm::vec3(w/2, h/2, 0.0f));
    three.setUV(glm::vec2(0.0f, 0.0f));
    three.setNormal(glm::vec3(1, 1, zDirNormal));
    vertices.push_back(three);

    Vertex four(glm::vec3(w/2, -h/2, 0.0f));
    four.setUV(glm::vec2(0.0f, 1.0f));
    four.setNormal(glm::vec3(1, -1, zDirNormal));
    vertices.push_back(four);

    Vertex backOne(glm::vec3(-w/2, -h/2, 0.0f));
    backOne.setNormal(glm::vec3(-1, -1, -zDirNormal));
    vertices.push_back(backOne);

    Vertex backTwo(glm::vec3(-w/2, h/2, 0.0f));
    backTwo.setNormal(glm::vec3(-1, 1, -zDirNormal));
    vertices.push_back(backTwo);

    Vertex backThree(glm::vec3(w/2, h/2, 0.0f));
    backThree.setNormal(glm::vec3(1, 1, -zDirNormal));
    vertices.push_back(backThree);

    Vertex backFour(glm::vec3(w/2, -h/2, 0.0f));
    backFour.setNormal(glm::vec3(1, -1, -zDirNormal));
    vertices.push_back(backFour);

    
    std::vector<uint32_t> indices;
    
    indices.push_back(3);
    indices.push_back(1);
    indices.push_back(0);

    indices.push_back(2);
    indices.push_back(1);
    indices.push_back(3);

    indices.push_back(4);
    indices.push_back(5);
    indices.push_back(7);

    indices.push_back(7);
    indices.push_back(5);
    indices.push_back(6);
    
    std::unique_ptr<Model> m = std::make_unique<Model>(vertices, indices, id);
    
    BoundingBox bbox;
    
    for (uint16_t i=0;i<4;i++) {
        Vertex v = vertices[i];
        glm::vec3 pos(v.getPosition());
        
        bbox.min.x = std::min(pos.x, bbox.min.x);
        bbox.min.y = std::min(pos.y, bbox.min.y);
        bbox.min.z = std::min(pos.z, bbox.min.z);
        
        bbox.max.x = std::max(pos.x, bbox.max.x);
        bbox.max.y = std::max(pos.y, bbox.max.y);
        bbox.max.z = std::max(pos.z, bbox.max.z);
    }
    
    bbox.min.z -= 0.1;
    bbox.max.z += 0.1;
    
    m->getMeshes()[0].setBoundingBox(bbox);
    m->calculateBoundingBoxForModel(DEBUG_BBOX);
        
    return m.release();
}

VkImageView Models::findTextureImageViewById(int id) {
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

std::vector<std::unique_ptr<Model>> & Models::getModels() {
    return this->models;
}

Model * Models::findModel(std::string id) {
    for (auto & m : this->models) {
        if (m->getId().compare(id) == 0) return m.get();
    }
    return nullptr;
}

void Model::setSsboOffset(VkDeviceSize offset) {
    this->ssboOffset = offset;
}

VkDeviceSize Model::getSsboOffset() {
    return this->ssboOffset;
}


TextureInformation Model::addTextures(const aiMaterial * mat) {
    TextureInformation textureInfo;
    
    if (mat->GetTextureCount(aiTextureType_AMBIENT) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_AMBIENT, 0, &str);
        
        if (str.length > 0) this->correctTexturePath(str.data);
        textureInfo.ambientTextureLocation = this->file.parent_path() / std::filesystem::path(str.C_Str());
    }

    if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_DIFFUSE, 0, &str);

        if (str.length > 0) this->correctTexturePath(str.data);
        textureInfo.diffuseTextureLocation = this->file.parent_path() / std::filesystem::path(str.C_Str());
    }

    if (mat->GetTextureCount(aiTextureType_SPECULAR) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_SPECULAR, 0, &str);

        if (str.length > 0) this->correctTexturePath(str.data);
        textureInfo.specularTextureLocation = this->file.parent_path() / std::filesystem::path(str.C_Str());
    }
    
    if (mat->GetTextureCount(aiTextureType_NORMALS) > 0) {
        aiString str;
        mat->GetTexture(aiTextureType_NORMALS, 0, &str);

        if (str.length > 0) this->correctTexturePath(str.data);
        textureInfo.normalTextureLocation = this->file.parent_path() / std::filesystem::path(str.C_Str());
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

Model::Model(std::string id) {
    this->id = id;
}

Model::~Model() {}

void Models::clear() {
    this->models.clear();
    this->textures.clear();
}

std::map< std::string, std::unique_ptr< Texture >>& Models::getTextures()
{
    return this->textures;
}

std::vector<std::string>  Models::getModelIds() {
    std::vector<std::string> modelIds;
    for (auto & m : this->models) {
        modelIds.push_back(m->getId());
    }
    return modelIds;
}

void Models::setColor(glm::vec4 color) {
    for (auto & model : this->models) {
        model->setColor(color);
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

int Texture::getId() {
    return this->id;
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

void Texture::setId(const int & id) {
    this->id = id;
}

void Texture::setType(const std::string & type) {
    this->type = type;
}

void Texture::setPath(const std::filesystem::path & path) {
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
        std::cout << "Loading texture: " << this->path << std::endl;
        this->textureSurface = IMG_Load(this->path.c_str());
        if (this->textureSurface != nullptr) {
            if (!this->readImageFormat()) {
                std::cout << "Unsupported Texture Format: " << this->path << std::endl;
            } else if (this->getSize() != 0) {
                this->valid = true;
            }
        } else std::cout << "Failed to load texture: " << this->path << std::endl;
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

Texture::Texture(bool empty,  VkExtent2D extent) {
    if (empty) {
        this->textureSurface = SDL_CreateRGBSurface(0,extent.width, extent.height, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);

        this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        
        this->loaded = true;
        this->valid = this->textureSurface != nullptr;
    }
}

Texture::Texture(SDL_Surface * surface) {
    if (surface != nullptr) {
        this->textureSurface = surface;
        this->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
        
        this->loaded = true;
        this->valid = true;
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
