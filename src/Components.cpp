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

glm::mat4 Component::getModelMatrix(bool includeRotation) {
    glm::mat4 transformation = glm::mat4(1.0f);

    transformation = glm::translate(transformation, this->position);

    if (includeRotation) {
        if (this->rotation.x != 0.0f) transformation = glm::rotate(transformation, this->rotation.x, glm::vec3(1, 0, 0));
        if (this->rotation.y != 0.0f) transformation = glm::rotate(transformation, this->rotation.y, glm::vec3(0, 1, 0));
        if (this->rotation.z != 0.0f) transformation = glm::rotate(transformation, this->rotation.z, glm::vec3(0, 0, 1));
    }

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
    for (auto & allComponentsPerModel : this->components) {
        auto & allComps = allComponentsPerModel.second;
        for (auto & c : allComps) {
            BoundingBox compBbox = c->getModel()->getBoundingBox();
            
            if (compBbox.min == INFINITY_VECTOR3 || compBbox.max == NEGATIVE_INFINITY_VECTOR3) continue;
            
            const glm::mat4 inverseModelMatrix = glm::inverse(c->getModelMatrix(true));
            BoundingBox inverseModelBbox = {
                inverseModelMatrix * glm::vec4(bbox.min,1),
                inverseModelMatrix * glm::vec4(bbox.max,1)
            };
            inverseModelBbox.min.x = std::min(inverseModelBbox.min.x, inverseModelBbox.max.x);
            inverseModelBbox.min.y = std::min(inverseModelBbox.min.y, inverseModelBbox.max.y);
            inverseModelBbox.min.z = std::min(inverseModelBbox.min.z, inverseModelBbox.max.z);
            inverseModelBbox.max.x = std::min(inverseModelBbox.min.x, inverseModelBbox.max.x);
            inverseModelBbox.max.y = std::min(inverseModelBbox.min.y, inverseModelBbox.max.y);
            inverseModelBbox.max.z = std::min(inverseModelBbox.min.z, inverseModelBbox.max.z);
            
            if (this->checkBboxIntersection(inverseModelBbox, compBbox)) {
                bool hitBBox = false;
                
                for (auto & m : c->getModel()->getMeshes()) {
                    if (m.isBoundingBox()) continue;
                    
                    compBbox = m.getBoundingBox();
                    
                    if (this->checkBboxIntersection(inverseModelBbox, compBbox)) {
                        if (m.getName().compare("opening") == 0 ||
                            m.getName().compare("inside") == 0
                        ) {
                            return false;
                        }
                        hitBBox = true;
                    }
                }
                
                if (hitBBox) return true;
            }
        }
    }
    
    return false;
}

bool Components::checkBboxIntersection(const BoundingBox & bbox1, const BoundingBox & bbox2) {
    bool intersectsAlongX = 
        (bbox1.min.x >= bbox2.min.x && bbox1.min.x <= bbox2.max.x) ||
        (bbox1.max.x >= bbox2.min.x && bbox1.max.x <= bbox2.max.x) ||
        (bbox1.min.x < bbox2.min.x && bbox1.max.x > bbox2.max.x);
    bool intersectsAlongY = 
        (bbox1.min.y >= bbox2.min.y && bbox1.min.y <= bbox2.max.y) ||
        (bbox1.max.y >= bbox2.min.y && bbox1.max.y <= bbox2.max.y) ||
        (bbox1.min.y < bbox2.min.y && bbox1.max.y > bbox2.max.y);

    bool intersectsAlongZ = 
        (bbox1.min.z >= bbox2.min.z && bbox1.min.z <= bbox2.max.z) ||
        (bbox1.max.z >= bbox2.min.z && bbox1.max.z <= bbox2.max.z) ||
        (bbox1.min.z < bbox2.min.z && bbox1.max.z > bbox2.max.z);
   
    return intersectsAlongX && intersectsAlongY && intersectsAlongZ;
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

