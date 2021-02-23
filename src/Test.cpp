
#include "includes/graphics.h"

class Test final {

private:
    const std::string APP_NAME = "Vulkan Test";
    Graphics & graphics = Graphics::instance();

    bool loadModels() {
        if (!this->graphics.isActive()) return false;

        const std::vector<Vertex> vertices = {
            Vertex(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f)),
            Vertex(glm::vec3(0.5f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
            Vertex(glm::vec3(0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            Vertex(glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f))
        };
        const std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0
        };
        std::vector<Mesh> meshes;
        meshes.push_back(Mesh(vertices, indices));

        Model rectangle(meshes);

        this->graphics.addModel(rectangle);

        Model teapot("/opt/projects/VulkanTest/res/models/", "teapot.obj");
        teapot.setColor(glm::vec3(0,1,0));
        this->graphics.addModel(teapot);

        Model rock("/opt/projects/VulkanTest/res/models/", "batman.obj");
        this->graphics.addModel(rock);

        return true;
    }

public:
    void run() {
        this->graphics.init(this->APP_NAME, VK_MAKE_VERSION(1,0,0));
        if (this->graphics.isActive()) {
            this->graphics.listVkExtensions();
            this->graphics.listVkLayerNames();
            this->graphics.listVkPhysicalDevices();

            std::unique_ptr<Camera> camera(Camera::instance(glm::vec3(0.0f, 4.0f, -10.0f)));

            auto windowSize = this->graphics.getWindowSize();
            float width = std::get<0>(windowSize);
            float height = std::get<1>(windowSize);
            Camera::instance()->setPerspective(45.0f, width/height);
            
            SDL_Event e;
            SDL_StartTextInput();

            bool quit = false;

            if (!this->loadModels()) {
                std::cerr << "Failed to Load Models" << std::endl;
            }

            if (!this->graphics.updateSwapChain()) {
                return;
            }

            float u = 0.0f;
            while(!quit) {
                while (SDL_PollEvent(&e) != 0) {
                    switch(e.type) {
                        case SDL_WINDOWEVENT:
                            if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                                this->graphics.updateSwapChain();
                            }
                            break;
                        case SDL_KEYDOWN:
                            switch (e.key.keysym.scancode) {
                                case SDL_SCANCODE_W:                                    
                                    Camera::instance()->move(Camera::KeyPress::UP, true, 0.5f);
                                    break;
                                case SDL_SCANCODE_S:
                                    Camera::instance()->move(Camera::KeyPress::DOWN, true, 0.5f);
                                    break;
                                case SDL_SCANCODE_A:
                                    Camera::instance()->move(Camera::KeyPress::LEFT, true, 0.5f);
                                    break;
                                case SDL_SCANCODE_D:
                                    Camera::instance()->move(Camera::KeyPress::RIGHT, true, 0.5f);
                                    break;
                                case SDL_SCANCODE_M:
                                    if (u > -100)  { u-= 1.0f; } 
                                    else u =0.0f;
                                    this->graphics.getModels().setPosition(0,0,u);
                                    this->graphics.renderScene();
                                    break;                                
                                case SDL_SCANCODE_F:
                                    this->graphics.toggleWireFrame();
                                    break;                                
                                case SDL_SCANCODE_Q:
                                    quit = true;
                                    break;
                                default:
                                    break;
                            };
                            break;
                        case SDL_KEYUP:
                            switch (e.key.keysym.scancode) {
                                case SDL_SCANCODE_W:
                                    Camera::instance()->move(Camera::KeyPress::UP);
                                    break;
                                case SDL_SCANCODE_S:
                                    Camera::instance()->move(Camera::KeyPress::DOWN);
                                    break;
                                case SDL_SCANCODE_A:
                                    Camera::instance()->move(Camera::KeyPress::LEFT);
                                    break;
                                case SDL_SCANCODE_D:
                                    Camera::instance()->move(Camera::KeyPress::RIGHT);
                                    break;
                                default:
                                    break;
                            };
                            break;
                        case SDL_MOUSEMOTION:
                            if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
                                Camera::instance()->updateDirection(
                                        static_cast<float>(e.motion.xrel),
                                        static_cast<float>(e.motion.yrel), 0.005f);
                            }
                            break;
                        case SDL_MOUSEBUTTONUP:
                            SDL_SetRelativeMouseMode(SDL_GetRelativeMouseMode() == SDL_TRUE ? SDL_FALSE : SDL_TRUE);
                            break;
                        case SDL_QUIT:
                            quit = true;
                            break;
                    }
                }

                this->graphics.drawFrame();
            }

            SDL_StopTextInput();
        }
    }
};

int main(int argc, char **argv) {
    Test vulkanTest;
    vulkanTest.run();

    return 0;
}
