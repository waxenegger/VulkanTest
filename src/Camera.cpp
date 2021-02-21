#include "includes/camera.h"

void Camera::updateViewMatrix() {
    glm::mat4 rotM = glm::mat4(1.0f);
    glm::mat4 transM;

    rotM = glm::rotate(rotM, glm::radians(rotation.x * (this->flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec3 translation = position;

    if (this->flipY) {
        translation.y *= -1.0f;
    }
    transM = glm::translate(glm::mat4(1.0f), translation);

    if (type == CameraType::firstperson) {
        this->view = rotM * transM;
    } else {
        this->view = transM * rotM;
    }

    viewPos = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

    updated = true;
};

bool Camera::moving()
{
    return this->keys.left || this->keys.right || this->keys.up || this->keys.down;
}

void Camera::setPerspective(float degrees, float aspect)
{
    this->perspective = glm::perspective(glm::radians(degrees), aspect, 0.1f, 10000.0f);
    if (this->flipY) {
        this->perspective[1][1] *= -1.0f;
    }
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

void Camera::setSpeed(float speed) {
    this->speed = speed;
}

void Camera::update(float deltaTime) {
    updated = false;
    if (type == CameraType::firstperson) {
        if (moving()) {
            glm::vec3 camFront;
            camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
            camFront.y = sin(glm::radians(rotation.x));
            camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
            camFront = glm::normalize(camFront);

            float moveSpeed = deltaTime * speed;

            if (this->keys.up) position += camFront * moveSpeed;
            if (this->keys.down) position -= camFront * moveSpeed;
            if (this->keys.left) position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
            if (this->keys.right) position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;

            this->updateViewMatrix();
        }
    }
};

ModelViewProjection Camera::getModelViewProjection() {
    return ModelViewProjection{
        glm::mat4(1.0f),
        this->view,
        this->perspective
    };
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
    if (Camera::singleton == nullptr) return nullptr;
    return Camera::singleton;
}

Camera * Camera::singleton = nullptr;


