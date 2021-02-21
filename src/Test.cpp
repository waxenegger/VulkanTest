
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

            std::unique_ptr<Camera> camera(Camera::instance(glm::vec3(0.0f, 0.0f, -10.0f)));
            this->graphics.getRenderContext().camera = camera.get();
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


            while(!quit) {
                while (SDL_PollEvent(&e) != 0) {
                    switch(e.type) {
                        case SDL_WINDOWEVENT:
                            if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
                                this->graphics.updateSwapChain();
                            }
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
