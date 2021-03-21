#include "includes/components.h"

Component::Component() {}

Component::Component(Model * model) {
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

Component * Components::addComponent(Component * component) {
    if (component == nullptr) return nullptr;
    
    std::unique_ptr<Component> componentPtr(component);
    
    const std::string modelLocation = component->hasModel() ? component->getModel()->getPath() : "";
    
    std::map<std::string, std::vector<std::unique_ptr<Component>>>::iterator it = this->components.find(modelLocation);
    if (it != this->components.end()) {
        it->second.push_back(std::move(componentPtr));
    } else {
        std::vector<std::unique_ptr<Component>> allComponentsPerModel;
        allComponentsPerModel.push_back(std::move(componentPtr));
        this->components[modelLocation] = std::move(allComponentsPerModel);
    }
    
    return component;
}

std::map<std::string, std::vector<std::unique_ptr<Component>>> & Components::getComponents() {
    return this->components;
}

void Components::initWithModelLocations(std::vector<std::string> modelLocations) {
    for(auto & l : modelLocations) {
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


std::vector<Component *> Components::getComponentsThatNeedUpdate() {
    std::vector<Component *> dirtyComponents;
    
    for (auto & entry : this->components) {
        auto & comps = entry.second;
        for (auto & c : comps) {
            if (c->needsSceneUpdate()) dirtyComponents.push_back(c.get());
        }
    }
    
    return dirtyComponents;
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

void Component::markSceneAsUpdated()
{
    this->sceneUpdate = false;
}

bool Component::needsSceneUpdate()
{
    return this->sceneUpdate;
}

glm::vec3 Component::getRotation() {
    return this->rotation;
}

void Component::setComponentOffset(VkDeviceSize offset) {
    if (this->componentOffsetHasBeenSet) return;
    this->componentOffset = offset;
    this->componentOffsetHasBeenSet = true;
}

VkDeviceSize Component::getComponentOffset() {
    return this->componentOffset;
}

