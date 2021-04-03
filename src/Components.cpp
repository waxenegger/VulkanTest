#include "includes/components.h"

Component::Component(std::string id) {
    this->id = id;
}

Component::Component(std::string id, Model * model) : Component(id) {
    this->model = model;
}

bool Component::hasModel() {
    return this->model != nullptr;
}

Model * Component::getModel() {
    return this->model;
}

glm::mat4 Component::getModelMatrix() {
    glm::mat4 transformation = glm::mat4(1.0f);

    transformation = glm::translate(transformation, this->position);

    if (this->rotation.x != 0.0f) transformation = glm::rotate(transformation, this->rotation.x, glm::vec3(1, 0, 0));
    if (this->rotation.y != 0.0f) transformation = glm::rotate(transformation, this->rotation.y, glm::vec3(0, 1, 0));
    if (this->rotation.z != 0.0f) transformation = glm::rotate(transformation, this->rotation.z, glm::vec3(0, 0, 1));

    return glm::scale(transformation, glm::vec3(this->scaleFactor));   
}

void Component::setPosition(float x, float y, float z) {
    this->setPosition(glm::vec3(x,y,z));
}

void Component::setPosition(glm::vec3 position) {
    this->position = position;
    this->sceneUpdate = true;
}

glm::vec3 Component::getPosition() {
    return this->position;
}   

void Component::rotate(int xAxis, int yAxis, int zAxis) {
    glm::vec3 rot;
    rot.x = glm::radians(static_cast<float>(xAxis));
    rot.y = glm::radians(static_cast<float>(yAxis));
    rot.z = glm::radians(static_cast<float>(zAxis));
    this->rotation += rot;
    this->sceneUpdate = true;
}

void Component::move(float xAxis, float yAxis, float zAxis) {
    this->position.x += xAxis;
    this->position.y += yAxis;
    this->position.z += zAxis;
    this->sceneUpdate = true;
}

void Component::setRotation(glm::vec3 rotation) {
    this->rotation = rotation;
    this->sceneUpdate = true;
}

void Component::scale(float factor) {
    if (factor <= 0) return;
    this->scaleFactor = factor;
    this->sceneUpdate = true;
}

std::string Component::getId() {
    return this->id;
}


Component * Components::addComponent(Component * component) {
    if (component == nullptr) return nullptr;
    
    std::unique_ptr<Component> componentPtr(component);
    
    const std::string modelId = component->hasModel() ? component->getModel()->getId() : "";
    
    std::map<std::string, std::vector<std::unique_ptr<Component>>>::iterator it = this->components.find(modelId);
    if (it != this->components.end()) {
        it->second.push_back(std::move(componentPtr));
    } else {
        std::vector<std::unique_ptr<Component>> allComponentsPerModel;
        allComponentsPerModel.push_back(std::move(componentPtr));
        this->components[modelId] = std::move(allComponentsPerModel);
    }
    
    return component;
}

std::map<std::string, std::vector<std::unique_ptr<Component>>> & Components::getComponents() {
    return this->components;
}

void Components::initWithModelIds(std::vector<std::string> modelIds) {
    for(auto & l : modelIds) {
        this->components[l] = std::vector<std::unique_ptr<Component>>();
    }
}

Components::~Components() {
    this->components.clear();
}

bool Components::isSceneUpdateNeeded(bool reset)
{
    bool ret = false;
    for (auto & c : this->components) {
        for (auto & m : c.second) {
            if (m->needsSceneUpdate()) {
                ret = true;
            }
            if (reset) m->markSceneAsUpdated();
            else if (ret) return true;
        }
    }
    return ret;
}

std::vector<Component *> Components::getAllComponentsForModel(std::string model) {
    std::vector<Component *> allMeshProperties;
    
    std::map<std::string, std::vector<std::unique_ptr<Component>>>::iterator it = this->components.find(model);
    if (it == this->components.end()) return allMeshProperties;
    
    std::vector<std::unique_ptr<Component>> & comps = it->second;
    for (std::unique_ptr<Component> & comp : comps) {
        if (comp->hasModel()) allMeshProperties.push_back(comp.get());
    }
    
    return allMeshProperties;
}

bool Components::checkCollision(BoundingBox & bbox) {
    std::cout << "min => " << bbox.min.x << "|" << bbox.min.y << "|" << bbox.min.z << std::endl;
    std::cout << "max => " << bbox.max.x << "|" << bbox.max.y << "|" << bbox.max.z << std::endl;
    
    for (auto & allComponentsPerModel : this->components) {
        auto & allComps = allComponentsPerModel.second;
        for (auto & c : allComps) {
            BoundingBox compBbox = c->getBoundingBox();
            
            if (c->getId().compare("blender1") == 0) {
                std::cout << "SAT: " << c->getId() << std::endl;
                std::cout << "model => " << compBbox.min.x << "|" << compBbox.min.y << "|" << compBbox.min.z << std::endl;
                std::cout << "model => " << compBbox.max.x << "|" << compBbox.max.y << "|" << compBbox.max.z << std::endl;                
            }
            
            bool intersectsAlongX = 
                (bbox.min.x >= compBbox.min.x && bbox.min.x <= compBbox.max.x) ||
                (bbox.max.x >= compBbox.min.x && bbox.max.x <= compBbox.max.x);
            bool intersectsAlongY = 
                (bbox.min.y >= compBbox.min.y && bbox.min.y <= compBbox.max.y) ||
                (bbox.max.y >= compBbox.min.y && bbox.max.y <= compBbox.max.y);

            bool intersectsAlongZ = 
                (bbox.min.z >= compBbox.min.z && bbox.min.z <= compBbox.max.z) ||
                (bbox.max.z >= compBbox.min.z && bbox.max.z <= compBbox.max.z);
                
            if (intersectsAlongX && intersectsAlongY && intersectsAlongZ) {
                std::cout << "INTER MODEL: " << c->getId() << std::endl;
                std::cout << "model => " << compBbox.min.x << "|" << compBbox.min.y << "|" << compBbox.min.z << std::endl;
                std::cout << "model => " << compBbox.max.x << "|" << compBbox.max.y << "|" << compBbox.max.z << std::endl;                
                return true;
            }
        }
    }
    
    return false;
}

void Component::markSceneAsUpdated()
{
    this->sceneUpdate = false;
}

bool Component::isVisible() {
    return this->visible;
};

void Component::setVisible(bool visible) {
    this->visible = visible;
};

bool Component::needsSceneUpdate()
{
    return this->sceneUpdate;
}

glm::vec3 Component::getRotation() {
    return this->rotation;
}

BoundingBox Component::getBoundingBox() {
    BoundingBox & modelBbox = this->getModel()->getBoundingBox();
    glm::mat4 modelMatrix = this->getModelMatrix();
    
    glm::mat2x4 bbox;
    bbox[0] = modelMatrix * glm::vec4(modelBbox.min,1);
    bbox[1] = modelMatrix * glm::vec4(modelBbox.max,1);
    
    return {
        glm::vec3(bbox[0].x, bbox[0].y, bbox[0].z),
        glm::vec3(bbox[1].x, bbox[1].y, bbox[1].z)
    };
}


