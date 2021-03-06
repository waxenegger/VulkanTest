
#include "includes/game.h"

int main(int argc, char **argv) {
    std::unique_ptr<Game> vulkanTest = std::make_unique<Game>(argc > 1 ? argv[1] : "./");
    vulkanTest->init();
    vulkanTest->loop();
    
    return 0;
}
