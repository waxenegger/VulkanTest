#ifndef SRC_INCLUDES_CAMERA_H_
#define SRC_INCLUDES_CAMERA_H_

#include "shared.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

class ModelViewProjection final {
    public:
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 projection;
};

class Camera
{
    public:
        enum CameraType { lookat, firstperson };
        
    private:
        Camera(glm::vec3 position);

        static Camera * singleton;
        CameraType type = CameraType::lookat;

        glm::vec3 rotation = glm::vec3();
        glm::vec3 position = glm::vec3();
        glm::vec4 viewPos = glm::vec4();

        float speed = 1.0f;

        bool updated = false;
        bool flipY = false;

        glm::mat4 perspective;
        glm::mat4 view;

        struct
        {
            bool left = false;
            bool right = false;
            bool up = false;
            bool down = false;
        } keys;

        void updateViewMatrix();
        bool moving();

        public:
            void setPerspective(float degrees, float aspect);
            void setPosition(glm::vec3 position);
            void setRotation(glm::vec3 rotation);
            void rotate(glm::vec3 delta);
            void setTranslation(glm::vec3 translation);
            void translate(glm::vec3 delta);
            void setSpeed(float movementSpeed);
            void update(float deltaTime);
            ModelViewProjection getModelViewProjection();
            static Camera * instance(glm::vec3 pos);
            static Camera * instance();
};

struct RenderContext {
    std::vector<VkCommandBuffer> commandBuffers;
    VkPipelineLayout graphicsPipelineLayout = nullptr;
    Camera * camera = nullptr;
};

#endif
