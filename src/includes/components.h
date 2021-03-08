#ifndef SRC_INCLUDES_COMPONENTS_H_
#define SRC_INCLUDES_COMPONENTS_H_

#include "models.h"

class Component final {
    private:
        Model * model = nullptr;
        
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        float scaleFactor = 1.0f;

        ModelProperties properties {
            glm::mat4(1), -1, -1, -1, -1
        };
    public:
        Component();
        Component(Model * model);
        ModelProperties & getModelProperties();
        bool hasModel();
        Model * getModel();
        void setPosition(float x, float y, float z);
        void setPosition(glm::vec3 position);
        glm::vec3 getPosition();
        void setRotation(int xAxis = 0, int yAxis = 0, int zAxis = 0);
        void scale(float factor);
        glm::mat4 getModelMatrix();
};

class Components final {
    private:
        std::map<std::string, std::vector<std::unique_ptr<Component>>> components;

    public:
        Component * addComponent(Component * component);
        std::vector<ModelProperties> getAllPropertiesForModel(std::string model);
        void initWithModelLocations(std::vector<std::string> modelLocations);
        std::map<std::string, std::vector<std::unique_ptr<Component>>> & getComponents();
        ~Components();
};

#endif
