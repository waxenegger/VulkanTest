#include "includes/camera.h"

void Camera::updateViewMatrix() {
    glm::mat4 rotM = glm::mat4(1.0f);
    glm::mat4 transM;

    rotM = glm::rotate(rotM, this->rotation.x * (this->flipY ? -1.0f : 1.0f), glm::vec3(1.0f, 0.0f, 0.0f));    
    rotM = glm::rotate(rotM, this->rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    rotM = glm::rotate(rotM, this->rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 translation = this->position;

    if (this->flipY) {
        translation.y *= -1.0f;
    }
    transM = glm::translate(glm::mat4(1.0f), translation);

    if (type == CameraType::firstperson) {
        this->view = rotM * transM;
    } else {
        this->view = transM * rotM;
    }
    
    this->frustum.update(this->perspective * this->view);
};

glm::vec3 Camera::getPosition() {
    return this->position;
}

bool Camera::moving()
{
    return this->keys.left || this->keys.right || this->keys.up || this->keys.down;
}

void Camera::move(KeyPress key, bool isPressed, float deltaTime, float terrainHeight) {
    
    switch(key) {
        case LEFT:
            this->keys.left = isPressed;
            break;
        case RIGHT:
            this->keys.right = isPressed;
            break;
        case UP:
            this->keys.up = isPressed;
            break;
        case DOWN:
            this->keys.down = isPressed;
            break;
        default:
            break;
    }
    
    if (isPressed) this->update(deltaTime, terrainHeight);
}

void Camera::moveWithConstraints(std::function<bool(BoundingBox)> collisionCheck, KeyPress key, float deltaTime, float terrainHeight) {
    if (collisionCheck(Camera::instance()->getBoundingBox(key, deltaTime))) return;
    
    this->move(key, true, deltaTime, terrainHeight);
}

void Camera::setAspectRatio(float aspect) {
    this->aspect = aspect;
    this->setPerspective();
};

void Camera::setFovY(float degrees)
{
    this->fovy = degrees;
    this->setPerspective();
};

float Camera::getFovY()
{
    return this->fovy;
};

void Camera::setPerspective()
{
    this->perspective = glm::perspective(glm::radians(this->fovy), this->aspect, 0.001f, 100000.0f);
    if (this->flipY) {
        this->perspective[1][1] *= -1.0f;
    }
    this->updateViewMatrix();
};

void Camera::setPosition(glm::vec3 position) {
    this->position = position;
    this->updateViewMatrix();
}

void Camera::setRotation(glm::vec3 rotation) {
    this->rotation = rotation;
    this->updateViewMatrix();
}

void Camera::rotate(glm::vec3 delta) {
    this->rotation += delta;
    this->updateViewMatrix();
}

void Camera::setTranslation(glm::vec3 translation) {
    this->position = translation;
    this->updateViewMatrix();
};

void Camera::translate(glm::vec3 delta) {
    this->position += delta;
    this->updateViewMatrix();
}

glm::vec3 Camera::getCameraFront() {
    glm::vec3 camFront;
    
    camFront.x = -cos(this->rotation.x) * sin(this->rotation.y);
    camFront.y = sin(this->rotation.x);
    camFront.z = cos(this->rotation.x) * cos(rotation.y);
    camFront = glm::normalize(camFront);
    
    return camFront;
}

void Camera::update(float deltaTime, float terrainHeight) {
    if (type == CameraType::firstperson) {
        if (moving()) {
            glm::vec3 camFront = this->getCameraFront();
            float moveSpeed = deltaTime;

            if (this->keys.up) {
                this->position += camFront * moveSpeed;
            }
            if (this->keys.down) {
                this->position -= camFront * moveSpeed;
            }
            if (this->keys.left) {
                this->position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
            }
            if (this->keys.right) {
                this->position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
            }
            
            this->position.y = terrainHeight + CameraHeight;

            this->updateViewMatrix();
        }
    }
};

void Camera::updateDirection(const float deltaX, const float  deltaY, float deltaTime) {
    float moveSpeed = deltaTime;
    
    glm::vec3 rot(0.0f);
    rot.y = deltaX * moveSpeed;
    rot.x = -deltaY * moveSpeed;
        
    this->rotate(rot);
}

void Camera::setType(CameraType type) {
    this->type = type;
}

glm::mat4 Camera::getViewMatrix() {
    return this->view;
};

glm::mat4 Camera::getProjectionMatrix() {
    return this->perspective;
};

Camera::Camera(glm::vec3 position)
{
    setPosition(position);
}

Camera * Camera::instance(glm::vec3 position) {
    if (Camera::singleton == nullptr) Camera::singleton = new Camera(position);
    return Camera::singleton;
}

Camera * Camera::instance() {
    if (Camera::singleton == nullptr) Camera::singleton = new Camera(glm::vec3(0.0f));
    return Camera::singleton;
}

void Camera::destroy() {
    if (Camera::singleton != nullptr) {
        delete Camera::singleton;
        Camera::singleton = nullptr;
    }
}

bool Camera::isInFrustum(glm::vec3 pos) {
    return this->frustum.checkSphere(pos, 5.0f);
}

BoundingBox Camera::getBoundingBox(KeyPress key, float distance) {
    glm::vec3 camFront = this->getCameraFront();
    
    glm::vec3 pos = this->position;
    
    float buffer = 0.15;
    
    switch(key) {
        case LEFT:
            pos -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * distance;
            break;
        case RIGHT:
            pos += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * distance;
            break;
        case UP:
            pos += camFront * distance;
            break;
        case DOWN:
            pos -= camFront * distance;
            break;
        case NONE:
            break;
    }
    
    glm::vec3 deltaPos = pos - this->position;
    if (glm::abs(deltaPos.x) < buffer) deltaPos.x < 0 ? pos.x-= buffer + deltaPos.x : pos.x+= buffer - deltaPos.x;
    if (glm::abs(deltaPos.y) < buffer) deltaPos.y < 0 ? pos.y-= buffer + deltaPos.y : pos.y+= buffer - deltaPos.y; 
    if (glm::abs(deltaPos.z) < buffer) deltaPos.z < 0 ? pos.z-= buffer + deltaPos.z : pos.z+= buffer - deltaPos.z; 
    
    return {
        glm::vec3(-pos.x, pos.y, -pos.z),
        glm::vec3(-pos.x, pos.y, -pos.z)
    };
}

Camera * Camera::singleton = nullptr;
const float Camera::CameraHeight = 5.0f;

