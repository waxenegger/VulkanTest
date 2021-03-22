#include "includes/game.h"

Game::Game(std::string root) {
    this->root = root;
    if (this->root[static_cast<int>(root.length())-1] != '/') this->root.append("/");
}

void Game::init() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    
    this->graphics.init(this->APP_NAME, VK_MAKE_VERSION(1,1,0), this->root);
    
    if (!this->graphics.isActive()) return;
    
    this->graphics.listVkExtensions();
    this->graphics.listPhysicalDeviceExtensions();
    this->graphics.listVkLayerNames();
    this->graphics.listVkPhysicalDevices();

    this->camera = Camera::instance(glm::vec3(0.0f, 4.0f, -10.0f));

    VkExtent2D windowSize = this->graphics.getWindowExtent();
    Camera::instance()->setAspectRatio(static_cast<float>(windowSize.width) / windowSize.height);
    
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

std::string Game::getRoot() { 
    return this->root; 
}


bool Game::loadModels() {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    if (!this->graphics.isActive()) return false;

    bool ret = false; 
    
    const std::vector<Vertex> vertices = {
        Vertex(glm::vec3(-0.5f, -0.5f, 5.0f), glm::vec3(1.0f, 1.0f, 1.0f)),
        Vertex(glm::vec3(0.5f, -0.5f, 5.0f), glm::vec3(1.0f, 1.0f, 1.0f)),
        Vertex(glm::vec3(0.5f, 0.5f, 5.0f), glm::vec3(1.0f, 1.0f, 1.0f)),
        Vertex(glm::vec3(-0.5f, 0.5f, 5.0f), glm::vec3(1.0f, 1.0f, 1.0f)),
        Vertex(glm::vec3(-0.5f, -0.5f, 5.0f), glm::vec3(0.0f, 1.0f, 1.0f)),
        Vertex(glm::vec3(0.5f, -0.5f, 5.0f), glm::vec3(0.0f, 1.0f, 1.0f)),
        Vertex(glm::vec3(0.5f, 0.5f, 5.0f), glm::vec3(0.0f, 1.0f, 1.0f)),
        Vertex(glm::vec3(-0.5f, 0.5f, 5.0f), glm::vec3(0.0f, 1.0f, 1.0f))

    };
    const std::vector<uint32_t> indices = {
        0, 3, 2, 2, 1, 0,
        4, 5, 6, 6, 7, 4
    };
    
    std::array<Model *, 5> models = {
        new Model(vertices, indices, "quad"),
        new Model("/opt/projects/VulkanTest/res/models/", "teapot.obj"),
        new Model("/opt/projects/VulkanTest/res/models/", "nanosuit.obj"),
        new Model("/opt/projects/VulkanTest/res/models/", "batman.obj"),
        new Model("/opt/projects/VulkanTest/res/models/", "woolly-mammoth-150k.obj")
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

    this->graphics.prepareComponents();
    
    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "loaded models: " << time_span.count() <<  std::endl;
    
    return ret;
}

bool Game::addComponents() {
    if (!this->graphics.isActive()) return false;
    
    bool ret = true;
    Component * quad = this->graphics.addModelComponent("quad");
    if (quad == nullptr) ret = false;
    else {
        quad->setPosition(glm::vec3(0, 4, 0));
        quad->scale(10);
    }

    Component * teapot = this->graphics.addModelComponent("/opt/projects/VulkanTest/res/models/teapot.obj");
    if (teapot == nullptr) ret = false;
    else {
        teapot->setPosition(glm::vec3(0, 4, 0));
        teapot->scale(1);
    }

    for (int x=-10;x<10;x+=2) {
        for (int z=-10;z<10;z+=2) {
            Component * batman = this->graphics.addModelComponent("/opt/projects/VulkanTest/res/models/batman.obj");
            if (batman == nullptr) ret = false;
            else batman->setPosition(glm::vec3(x,0,z));
        }        
    }

    Component * nanosuit = this->graphics.addModelComponent("/opt/projects/VulkanTest/res/models/nanosuit.obj");
    if (nanosuit == nullptr) ret = false;
    else {
        nanosuit->setPosition(glm::vec3(0, 10, 0));
        nanosuit->scale(0.5);
    }

    Component * mammoth = this->graphics.addModelComponent("/opt/projects/VulkanTest/res/models/woolly-mammoth-150k.obj");
    if (mammoth == nullptr) ret = false;
    else {
        mammoth->setPosition(glm::vec3(-7, 4, 0));
        mammoth->rotate(0, 60, 0);
        mammoth->scale(2);
    }

    return ret;
}


void Game::loop() {
    if (!this->initialized) return;
    
    SDL_StartTextInput();

    bool quit = false;
    
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    this->graphics.setLastTimeMeasure(start);

    std::thread rotationThread([this, &quit]() {
        std::chrono::high_resolution_clock::time_point rotationStart = std::chrono::high_resolution_clock::now();
        while(!quit) {
            std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - rotationStart;
            if (time_span.count() >= 125) {
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

    std::thread inputThread([this, &quit]() {
        SDL_Event e;
        bool isFullScreen = false;
        bool needsRestoreAfterFullScreen = false;

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
        }
    });
    inputThread.detach();
    
    while(!quit) {
        this->graphics.drawFrame();        
    }

    SDL_StopTextInput();
}

Game::~Game() {
    if (this->camera != nullptr) this->camera->destroy();
}

