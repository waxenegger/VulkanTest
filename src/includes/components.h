#ifndef SRC_INCLUDES_COMPONENTS_H_
#define SRC_INCLUDES_COMPONENTS_H_

#include "models.h"

class Component final {
    private:
        Model * model = nullptr;
        
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        float scaleFactor = 1.0f;
        
        bool sceneUpdate = false;
        VkDeviceSize componentOffset;
        bool componentOffsetHasBeenSet = false;
    public:
        Component();
        Component(Model * model);
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
        void setComponentOffset(VkDeviceSize offset);
        VkDeviceSize getComponentOffset();
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
        std::vector<Component *> getComponentsThatNeedUpdate();
        bool isSceneUpdateNeeded(bool reset = true);
};

#endif
