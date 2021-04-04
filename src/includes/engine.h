#ifndef SRC_INCLUDES_GAME_H_
#define SRC_INCLUDES_GAME_H_

#include "graphics.h"

class Engine final {
    private:
        bool initialized = false;
        std::filesystem::path root = "./";
        Camera * camera = nullptr;
        const std::string APP_NAME = "Vulkan Test";
        Graphics & graphics = Graphics::instance();
    public:
        Engine(std::filesystem::path root);
        void init();
        void loop();
        bool loadModels();
        bool addComponents();
        ~Engine();
};

#endif
