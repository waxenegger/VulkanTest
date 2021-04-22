#ifndef SRC_INCLUDES_GAME_H_
#define SRC_INCLUDES_GAME_H_

#include "graphics.h"

class Engine final {
    private:
        bool initialized = false;
        std::filesystem::path root = "./";
        std::unique_ptr<SpatialTree> rTree = std::unique_ptr<SpatialTree>(SpatialTree::instance());
        std::unique_ptr<Camera> camera = std::unique_ptr<Camera>(Camera::instance());
        const std::string APP_NAME = "Vulkan Test";
        Graphics & graphics = Graphics::instance();
    public:
        Engine(std::filesystem::path root);
        void init();
        void loop();
        bool loadModels();
        bool addComponents();
};

#endif
