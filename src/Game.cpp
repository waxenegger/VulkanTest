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
    this->camera->setAspectRatio(windowSize.width/windowSize.height);
    
    if (!this->loadModels()) {
         std::cerr << "Failed to Create Models" << std::endl;
         return;
    }
    
    if (!this->graphics.prepareModels()) {
         std::cerr << "Failed to Prepare Models" << std::endl;
        return;
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
        0, 1, 2, 2, 3, 0,
        4, 7, 6, 6, 5, 4
    };
    Model * quad = new Model(vertices, indices);
    this->graphics.addModel(quad);

    std::chrono::high_resolution_clock::time_point batmanStart = std::chrono::high_resolution_clock::now();

    Model * batman = new Model("/opt/projects/VulkanTest/res/models/", "batman.obj");
    batman->setPosition(0.0f, 0.0f, 10.0f);
    batman->scale(5);
    this->graphics.addModel(batman);

    Model * mammoth = new Model("/opt/projects/VulkanTest/res/models/", "woolly-mammoth-150k.obj");
    mammoth->setPosition(50,0,50);
    mammoth->scale(2);
    this->graphics.addModel(mammoth);

    std::chrono::duration<double, std::milli> time_span = std::chrono::high_resolution_clock::now() - batmanStart;
    std::cout << "load batman: " << time_span.count() <<  std::endl;

    for (int i=0;i<1;i++) {
        for (int j=0;j<1;j++) {
            Model * teapot = new Model("/opt/projects/VulkanTest/res/models/", "teapot.obj");
            teapot->setColor(glm::vec3(1.0f, 1.0f, 1.0f));

            glm::vec3 pos = teapot->getPosition();
            pos.z -= i * 10;
            pos.x -= j * 10;
            teapot->setPosition(pos);
            teapot->setPosition(glm::vec3(0,0,-15));
            //this->graphics.addModel(teapot);
        }
    }

    std::chrono::high_resolution_clock::time_point nanosuitStart = std::chrono::high_resolution_clock::now();

    //Model * nanosuit = new Model("/opt/projects/VulkanTest/res/models/", "nanosuit.obj");
    //this->graphics.addModel(nanosuit);

    time_span = std::chrono::high_resolution_clock::now() - nanosuitStart;
    std::cout << "load nanosuit: " << time_span.count() <<  std::endl;

    time_span = std::chrono::high_resolution_clock::now() - start;
    std::cout << "load models: " << time_span.count() <<  std::endl;
    
    return true;
}

void Game::loop() {
    if (!this->initialized) return;
    
    SDL_Event e;
    SDL_StartTextInput();

    bool quit = false;
    bool isFullScreen = false;
    bool needsRestoreAfterFullScreen = false;
    
    float u = 0.0f;
    while(!quit) {
        while (SDL_PollEvent(&e) != 0) {
            switch(e.type) {
                case SDL_WINDOWEVENT:
                    if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
                        e.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                        e.window.event == SDL_WINDOWEVENT_MINIMIZED ||
                        e.window.event == SDL_WINDOWEVENT_RESTORED) {
                            if (isFullScreen) SDL_SetWindowFullscreen(this->graphics.getSdlWindow(), SDL_TRUE);
                            this->graphics.updateSwapChain();
                            VkExtent2D windowSize = this->graphics.getWindowExtent();
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

        this->graphics.drawFrame();
    }

    SDL_StopTextInput();
}

Game::~Game() {
    if (this->camera != nullptr) this->camera->destroy();
}

