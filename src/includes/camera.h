#ifndef SRC_INCLUDES_CAMERA_H_
#define SRC_INCLUDES_CAMERA_H_

#include "threading.h"
#include "frustum.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

static constexpr float INF = std::numeric_limits<float>::infinity();
static constexpr float NEG_INF = -1 * std::numeric_limits<float>::infinity();
static constexpr glm::vec3 INFINITY_VECTOR3 = glm::vec3(INF);
static constexpr glm::vec3 NEGATIVE_INFINITY_VECTOR3 = glm::vec3(NEG_INF);
static constexpr glm::mat4 IDENTITY_MATRIX4 = glm::mat4(1.0f);

struct BoundingBox final {
    public:
        glm::vec3 min = glm::vec3(INF);
        glm::vec3 max = glm::vec3(NEG_INF);
};

struct ModelUniforms final {
    public:
        glm::mat4 viewMatrix;
        glm::mat4 projectionMatrix;
        glm::vec4 camera;
        glm::vec4 sun;
};

class Camera
{
    public:
        enum CameraType { lookat, firstperson };
        enum KeyPress { LEFT = 0, RIGHT = 1, UP = 2, DOWN = 3 };
        
    private:
        Camera(glm::vec3 position);

        static Camera * singleton;
        CameraType type = CameraType::firstperson;

        glm::vec3 rotation = glm::vec3();
        glm::vec3 position = glm::vec3();

        float aspect = 1.0f;
        float fovy = 45.0f;
        
        Frustum frustum;
        
        struct
        {
            bool left = false;
            bool right = false;
            bool up = false;
            bool down = false;
        } keys;

        bool flipY = true;
        
        glm::mat4 perspective = glm::mat4();
        glm::mat4 view = glm::mat4();

        void updateViewMatrix();
        bool moving();

        public:
            glm::vec3 getPosition();
            void setAspectRatio(float aspect);
            void setFovY(float degrees);
            float getFovY();
            void setPerspective();
            void setPosition(glm::vec3 position);
            void setRotation(glm::vec3 rotation);
            void rotate(glm::vec3 delta);
            void setTranslation(glm::vec3 translation);
            void translate(glm::vec3 delta);
            void update(float deltaTime);
            glm::mat4 getViewMatrix();
            glm::mat4 getProjectionMatrix();
            static Camera * instance(glm::vec3 pos);
            static Camera * instance();
            void setType(CameraType type);
            void move(KeyPress key, bool isPressed = false, float deltaTime = 1.0f);
            void updateDirection(const float deltaX, const float  deltaY, float deltaTime = 1.0f);
            void destroy();
            
            bool isInFrustum(glm::vec3 pos);
            BoundingBox getBoundingBox(KeyPress key, float distance);
            glm::vec3 getCameraFront();
};

#endif
