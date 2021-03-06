#include "includes/engine.h"

Engine::Engine(std::filesystem::path root) {
    if (!std::filesystem::exists(root)) {
        std::cerr << "Root Directory " << root << " does not exist!" << std::endl;
    }
    this->root = root;
}

void Engine::init() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    this->graphics.init(this->APP_NAME, VK_MAKE_VERSION(1,1,0), this->root);
    
    if (!this->graphics.isActive()) {
        std::cerr << "Failed to initialize Graphics Facility" << std::endl;
        return;
    }
    
    //this->graphics.listVkExtensions();
    //this->graphics.listPhysicalDeviceExtensions();
    //this->graphics.listVkLayerNames();
    //this->graphics.listVkPhysicalDevices();
    
    glm::vec3 cameraPosition = glm::vec3(-10.0f, 0.0, 10.0f);
    cameraPosition.y = this->graphics.getTerrainHeightAtPosition(cameraPosition) + Camera::CameraHeight;
    this->camera->setPosition(cameraPosition);

    VkExtent2D windowSize = this->graphics.getWindowExtent();
    this->camera->setAspectRatio(static_cast<float>(windowSize.width) / windowSize.height);
    
    if (!this->loadModels()) {
         std::cerr << "Failed to Create All Models" << std::endl;
    }

    if (!this->addComponents()) {
         std::cerr << "Failed to Create All Components" << std::endl;
    }

    if (!this->graphics.prepareModels()) {
         std::cerr << "Failed to Prepare Models" << std::endl;
    }    
    
    if (!this->graphics.updateSwapChain()) {
        return;
    }

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "initial wait till draw loop: " << time_span.count() <<  std::endl;
        
    this->initialized = true;
}

bool Engine::loadModels() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    if (!this->graphics.isActive()) return false;

    bool ret = true; 
    
    std::array<Model *, 8> models = {
        new Model("batman", this->graphics.getAppPath(MODELS) / "batman.obj"),
        new Model("teapot", this->graphics.getAppPath(MODELS) / "teapot.obj"),
        new Model("nanosuit", this->graphics.getAppPath(MODELS) / "nanosuit.obj"),
        new Model("cyborg", this->graphics.getAppPath(MODELS) / "cyborg.obj"),
        new Model("mammoth", this->graphics.getAppPath(MODELS) / "woolly-mammoth-150k.obj"),
        new Model("plane", this->graphics.getAppPath(MODELS) / "plane.obj"),
        new Model("contraption", this->graphics.getAppPath(MODELS) / "contraption.obj"),
        new Model("house", this->graphics.getAppPath(MODELS) / "house.obj")
    };
    
    for (auto * m : models) {
        if (m == nullptr) continue;
        
        if (m->hasBeenLoaded()) {
            this->graphics.addModel(m);
        } else {
            ret = false;
            delete m;
        }
    };
    
    this->graphics.addText("text", "FreeMono.ttf", "hello world", 50);

    this->graphics.prepareComponents();
    
    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "loaded models: " << time_span.count() <<  std::endl;
    
    return ret;
}

bool Engine::addComponents() {
    if (!this->graphics.isActive()) return false;
    
    bool ret = true;

    glm::vec3 housePosition = glm::vec3(15, 0, 10);
    housePosition.y = this->graphics.getTerrainHeightAtPosition(housePosition);

    Component * text = this->graphics.addComponentWithModel("text1", "text");
    if (text == nullptr) ret = false;
    else {
        text->setPosition(glm::vec3(0, this->graphics.getTerrainHeightAtPosition(housePosition)+20, -15));
        text->scale(5);
    }

    Component * house = this->graphics.addComponentWithModel("house1", "house");
    if (house == nullptr) ret = false;
    else {
        
        house->setPosition(housePosition);
        house->scale(3);
    }

    Component * teapot = this->graphics.addComponentWithModel("teatpot1", "teapot");
    if (teapot == nullptr) ret = false;
    else {
        teapot->setPosition(glm::vec3(24, housePosition.y + 2, 0));
        teapot->scale(1);
    }

    for (int x=-10;x<10;x+=2) {
        for (int z=-10;z<10;z+=2) {
            Component * batman = this->graphics.addComponentWithModel(
                std::string("batman" + std::to_string(x) + "_" + std::to_string(z)), "batman");
            if (batman == nullptr) ret = false;
            else batman->setPosition(glm::vec3(x, housePosition.y + 2,z));
        }        
    }
    
    Component * nanosuit = this->graphics.addComponentWithModel("nanosuit1", "nanosuit");
    if (nanosuit == nullptr) ret = false;
    else {
        nanosuit->setPosition(glm::vec3(0, 12, 0));
        nanosuit->scale(0.5);
    }

    Component * mammoth = this->graphics.addComponentWithModel("mammoth1", "mammoth");
    if (mammoth == nullptr) ret = false;
    else {
        mammoth->setPosition(glm::vec3(15, housePosition.y + 5, 0));
        mammoth->scale(2);
    }

    Component * contraption = this->graphics.addComponentWithModel("blender1", "contraption");
    if (contraption == nullptr) ret = false;
    else {
        contraption->setPosition(glm::vec3(0, this->graphics.getTerrainHeightAtPosition(housePosition) + 100, 100));
        contraption->rotate(0, 0, 0);
        contraption->scale(2);
    }

    Component * cyborg = this->graphics.addComponentWithModel("cyborg1", "cyborg");
    if (cyborg == nullptr) ret = false;
    else {
        cyborg->setPosition(glm::vec3(15, housePosition.y + 2, 10));
        cyborg->scale(1.5);
    }


    return ret;
}


void Engine::loop() {
    if (!this->initialized) return;
    
    SDL_StartTextInput();

    bool quit = false;
    
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    this->graphics.setLastTimeMeasure(start);

    /*
    std::thread rotationThread([this, &quit]() {
        std::chrono::high_resolution_clock::time_point rotationStart = std::chrono::high_resolution_clock::now();
        while(!quit) {
            std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - rotationStart;
            if (time_span.count() >= 5000) {
                auto & components = this->graphics.getComponents().getComponents();
                for (auto & c : components) {
                    auto & allCompsPerModel =  c.second;
                    for (auto & cp : allCompsPerModel) {
                        cp->rotate(0,5,0);
                    }
                }
                rotationStart = std::chrono::high_resolution_clock::now();

            }
        }
    });
    rotationThread.detach();
    */

    std::thread inputThread([this, &quit]() {
        SDL_Event e;
        bool isFullScreen = false;
        bool needsRestoreAfterFullScreen = false;

        float walkingSpeed = 0.5;
        
        std::function<bool(BoundingBox)> checkkCollisionFunc = std::bind(&Graphics::checkCollision, &this->graphics, std::placeholders::_1);
        
        while(!quit) {
            while (SDL_PollEvent(&e) != 0) {
                switch(e.type) {
                    case SDL_WINDOWEVENT:
                        if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
                            e.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                            e.window.event == SDL_WINDOWEVENT_MINIMIZED ||
                            e.window.event == SDL_WINDOWEVENT_RESTORED) {
                                if (isFullScreen) SDL_SetWindowFullscreen(this->graphics.getSdlWindow(), SDL_TRUE);
                        }
                        break;
                    case SDL_KEYDOWN:
                        switch (e.key.keysym.scancode) {
                            case SDL_SCANCODE_W:
                                this->camera->moveWithConstraints(
                                    checkkCollisionFunc, Camera::KeyPress::UP, walkingSpeed, 
                                        this->graphics.getTerrainHeightAtPosition(this->camera->getPosition()));
                                break;
                            case SDL_SCANCODE_S:
                                this->camera->moveWithConstraints(
                                    checkkCollisionFunc, Camera::KeyPress::DOWN, walkingSpeed,
                                        this->graphics.getTerrainHeightAtPosition(this->camera->getPosition()));
                                break;
                            case SDL_SCANCODE_A:
                               this->camera->moveWithConstraints(
                                    checkkCollisionFunc, Camera::KeyPress::LEFT, walkingSpeed,
                                        this->graphics.getTerrainHeightAtPosition(this->camera->getPosition()));
                                break;
                            case SDL_SCANCODE_D:
                                this->camera->moveWithConstraints(
                                    checkkCollisionFunc, Camera::KeyPress::RIGHT, walkingSpeed,
                                        this->graphics.getTerrainHeightAtPosition(this->camera->getPosition()));
                                break;
                            case SDL_SCANCODE_F:
                                this->graphics.toggleWireFrame();
                                break;                                
                            case SDL_SCANCODE_F12:
                                isFullScreen = !isFullScreen;
                                if (isFullScreen) {
                                    if (SDL_GetWindowFlags(this->graphics.getSdlWindow()) & SDL_WINDOW_MAXIMIZED) {
                                        SDL_SetWindowFullscreen(this->graphics.getSdlWindow(), SDL_TRUE);
                                    } else {
                                        needsRestoreAfterFullScreen = true;
                                        SDL_MaximizeWindow(this->graphics.getSdlWindow());
                                    }
                                } else {
                                    SDL_SetWindowFullscreen(this->graphics.getSdlWindow(), SDL_FALSE);
                                    if (needsRestoreAfterFullScreen) {
                                        SDL_RestoreWindow(this->graphics.getSdlWindow());
                                        needsRestoreAfterFullScreen = false;
                                    }
                                }
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
                                this->camera->move(Camera::KeyPress::UP);
                                break;
                            case SDL_SCANCODE_S:
                                this->camera->move(Camera::KeyPress::DOWN);
                                break;
                            case SDL_SCANCODE_A:
                                this->camera->move(Camera::KeyPress::LEFT);
                                break;
                            case SDL_SCANCODE_D:
                                this->camera->move(Camera::KeyPress::RIGHT);
                                break;
                            default:
                                break;
                        };
                        break;
                    case SDL_MOUSEMOTION:
                        if (SDL_GetRelativeMouseMode() == SDL_TRUE) {
                            this->camera->updateDirection(
                                static_cast<float>(e.motion.xrel),
                                static_cast<float>(e.motion.yrel), 0.005f);
                        }
                        break;
                    case SDL_MOUSEWHEEL:
                    {
                        const Sint32 delta = e.wheel.y * (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL ? 1 : -1);
                        float newFovy = this->camera->getFovY() - delta * 2;
                        if (newFovy < 1) newFovy = 1;
                        else if (newFovy > 45) newFovy = 45;
                        this->camera->setFovY(newFovy);
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
        }
    });
    inputThread.detach();
    
    while(!quit) {
        this->graphics.drawFrame();        
    }
    
    SDL_StopTextInput();
}
