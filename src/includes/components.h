#ifndef SRC_INCLUDES_COMPONENTS_H_
#define SRC_INCLUDES_COMPONENTS_H_

#include "models.h"

class Component final {
    private:
        Model * model = nullptr;
        
        std::string id = "";
        
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        float scaleFactor = 1.0f;
        
        bool visible = true;
        bool sceneUpdate = false;
    public:
        Component(std::string id);
        Component(std::string id, Model * model);
        MeshProperties & getModelProperties();
        bool hasModel();
        Model * getModel();
        void setPosition(float x, float y, float z);
        void setPosition(glm::vec3 position);
        glm::vec3 getPosition();
        void setRotation(glm::vec3 rotation);
        void rotate(int xAxis = 0, int yAxis = 0, int zAxis = 0);
        void move(float xAxis, float yAxis, float zAxis);
        void scale(float factor);
        glm::mat4 getModelMatrix();
        bool needsSceneUpdate();
        void markSceneAsUpdated();
        glm::vec3 getRotation();
        bool isVisible();
        void setVisible(bool visible);
};

class Components final {
    private:
        std::map<std::string, std::vector<std::unique_ptr<Component>>> components;

    public:
        Component * addComponent(Component * component);
        std::vector<Component *> getAllComponentsForModel(std::string model);
        void initWithModelLocations(std::vector<std::string> modelLocations);
        std::map<std::string, std::vector<std::unique_ptr<Component>>> & getComponents();
        ~Components();
        std::vector<Component *> getSsboComponentsThatNeedUpdate();
        bool isSceneUpdateNeeded(bool reset = true);
};

#endif
