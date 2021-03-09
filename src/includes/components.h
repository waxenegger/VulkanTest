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
        bool ssboUpdate = false;
        int ssboIndex = -1;
    public:
        Component();
        Component(Model * model);
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
        bool needsSsboUpdate();
        bool needsSceneUpdate();
        void markSsboAsNotDirty();
        void markSceneAsUpdated();
        void setSsboIndex(int index);
        int getSsboIndex();
        glm::vec3 getRotation();

};

class Components final {
    private:
        std::map<std::string, std::vector<std::unique_ptr<Component>>> components;

    public:
        Component * addComponent(Component * component);
        std::vector<Component *> getAllComponentsForModel(std::string model, bool hasModel = false);
        void initWithModelLocations(std::vector<std::string> modelLocations);
        std::map<std::string, std::vector<std::unique_ptr<Component>>> & getComponents();
        ~Components();
        std::vector<Component *> getSsboComponentsThatNeedUpdate();
        bool isSceneUpdateNeeded(bool reset = true);
};

#endif
