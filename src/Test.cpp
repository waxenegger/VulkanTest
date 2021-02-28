
#include "includes/graphics.h"

class Test final {

private:
    const std::string APP_NAME = "Vulkan Test";
    Graphics & graphics = Graphics::instance();

    bool addModels() {
        std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

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
        this->graphics.addModel(vertices, indices);

        std::chrono::high_resolution_clock::time_point batmanStart = std::chrono::high_resolution_clock::now();

        Model * batman = new Model("/opt/projects/VulkanTest/res/models/", "batman.obj");
        batman->setPosition(0,0,0);
        this->graphics.addModel(batman);

        std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - batmanStart;
        std::cout << "load batman: " << time_span.count() <<  std::endl;

        for (int i=0;i<1;i++) {
            for (int j=0;j<1;j++) {
                Model * teapot = new Model("/opt/projects/VulkanTest/res/models/", "teapot.obj");
                teapot->setColor(glm::vec3(1.0f, 0.0f, 0.0f));

                glm::vec3 pos = teapot->getPosition();
                pos.z -= i * 10;
                pos.x -= j * 10;
                teapot->setPosition(pos);
                teapot->setPosition(glm::vec3(0,0,-15));
                this->graphics.addModel(teapot);
            }
        }

        std::chrono::high_resolution_clock::time_point nanosuitStart = std::chrono::high_resolution_clock::now();

        Model * nanosuit = new Model("/opt/projects/VulkanTest/res/models/", "nanosuit.obj");
        this->graphics.addModel(nanosuit);

        time_span = std::chrono::high_resolution_clock::now() - nanosuitStart;
        std::cout << "load nanosuit: " << time_span.count() <<  std::endl;

        time_span = std::chrono::high_resolution_clock::now() - start;
        std::cout << "load models: " << time_span.count() <<  std::endl;
        
        return true;
    }

public:
    void run() {
        std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

        this->graphics.init(this->APP_NAME, VK_MAKE_VERSION(1,0,0));
        if (this->graphics.isActive()) {
            this->graphics.listVkExtensions();
            this->graphics.listVkLayerNames();
            this->graphics.listVkPhysicalDevices();

            std::unique_ptr<Camera> camera(Camera::instance(glm::vec3(0.0f, 4.0f, -10.0f)));

            VkExtent2D windowSize = this->graphics.getWindowExtent();
            Camera::instance()->setAspectRatio(windowSize.width/windowSize.height);
            
            SDL_Event e;
            SDL_StartTextInput();

            bool quit = false;

            if (!this->addModels()) {
                std::cerr << "Failed to Load Models" << std::endl;
            }
            
            if (!this->graphics.prepareModels()) {
                return;
            }

            if (!this->graphics.updateSwapChain()) {
                return;
            }

            std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
            std::cout << "initial wait till draw loop: " << time_span.count() <<  std::endl;

            float u = 0.0f;
            while(!quit) {
                while (SDL_PollEvent(&e) != 0) {
                    switch(e.type) {
                        case SDL_WINDOWEVENT:
                            if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                                this->graphics.updateSwapChain();
                                
                                const VkExtent2D windowSize = this->graphics.getWindowExtent();
                                Camera::instance()->setAspectRatio(windowSize.width/windowSize.height);
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
                                    this->graphics.getModels().setPosition(0,u,0);
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
                        case SDL_MOUSEWHEEL:
                        {
                            const Sint32 delta = e.wheel.y * (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL ? 1 : -1);
                            float newFovy = Camera::instance()->getFovY() - delta * 2;
                            if (newFovy < 1) newFovy = 1;
                            else if (newFovy > 45) newFovy = 45;
                            Camera::instance()->setFovY(newFovy);
                            break;
                        }                            
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
