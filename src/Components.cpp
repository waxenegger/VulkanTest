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
}

glm::vec3 Component::getPosition() {
    return this->position;
}

void Component::setRotation(int xAxis, int yAxis, int zAxis) {
    this->rotation.x = glm::radians(static_cast<float>(xAxis));
    this->rotation.y = glm::radians(static_cast<float>(yAxis));
    this->rotation.z = glm::radians(static_cast<float>(zAxis));
}

void Component::scale(float factor) {
    if (factor <= 0) return;
    
    this->scaleFactor = factor;
}


ModelProperties & Component::getModelProperties() {
    this->properties.matrix = this->getModelMatrix();
    return this->properties;
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

void Components::initWithModelLocations(std::vector<std::string> modelLocations) {
    for(auto & l : modelLocations) {
        this->components[l] = std::vector<std::unique_ptr<Component>>();
    }
}

Components::~Components() {
    this->components.clear();
}

std::vector<ModelProperties> Components::getAllPropertiesForModel(std::string model) {
    std::vector<ModelProperties> allModelProperties;
    
    std::map<std::string, std::vector<std::unique_ptr<Component>>>::iterator it = this->components.find(model);
    if (it == this->components.end()) return allModelProperties;
    
    for (auto & comp : it->second) {
        allModelProperties.push_back(comp->getModelProperties());
    }
    
    return allModelProperties;
}


