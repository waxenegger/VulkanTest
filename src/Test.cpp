
#include "includes/graphics.h"

class Test final {

private:
    const std::string APP_NAME = "Vulkan Test";
    Graphics & graphics = Graphics::instance();

public:
    void run() {
        this->graphics.init(this->APP_NAME, VK_MAKE_VERSION(1,0,0));
        if (this->graphics.isActive()) {
            this->graphics.listVkExtensions();
            this->graphics.listVkLayerNames();
            this->graphics.listVkPhysicalDevices();

            SDL_Event e;
            SDL_StartTextInput();

            bool quit = false;

            if (!this->graphics.updateSwapChain()) {
                return;
            }

            while(!quit) {
                while (SDL_PollEvent(&e) != 0) {
                    switch(e.type) {
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
