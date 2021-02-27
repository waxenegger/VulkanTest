#ifndef SRC_INCLUDES_CAMERA_H_
#define SRC_INCLUDES_CAMERA_H_

#include "shared.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

struct ModelUniforms final {
    public:
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
};

struct ModelProperties final {
    public:
        glm::mat4 matrix;
        int ambientTexture = -1;
        int diffuseTexture = -1;
        int specularTexture = -1;
        int normalTexture = -1;
};

class Camera
{
    public:
        enum CameraType { lookat, firstperson };
        enum KeyPress { LEFT = 0, RIGHT = 1, UP = 2, DOWN = 3};
        
    private:
        Camera(glm::vec3 position);

        static Camera * singleton;
        CameraType type = CameraType::firstperson;

        glm::vec3 rotation = glm::vec3();
        glm::vec3 position = glm::vec3();
        
        glm::vec3 direction = glm::vec3(0.001f, 0.0f, -0.001f);

        float speed = 1.0f;
        struct
        {
            bool left = false;
            bool right = false;
            bool up = false;
            bool down = false;
        } keys;

        bool flipY = false;

        glm::mat4 perspective;
        glm::mat4 view;

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
            glm::mat4 getViewMatrix();
            glm::mat4 getProjectionMatrix();
            static Camera * instance(glm::vec3 pos);
            static Camera * instance();
            void setType(CameraType type);
            void move(KeyPress key, bool isPressed = false, float deltaTime = 1.0f);
            void updateDirection(const float deltaX, const float  deltaY, float deltaTime = 1.0f);
};

#endif
