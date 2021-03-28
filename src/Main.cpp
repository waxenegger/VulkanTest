
#include "includes/engine.h"

int main(int argc, char **argv) {
    std::filesystem::path root = argc > 1 ? argv[1] : "./";
    if (!std::filesystem::exists(root / "res/shaders") || !std::filesystem::is_directory(root / "res/shaders")) {
        std::cerr << "Parameter 1 has to be root directory that contains resources folder!" << std::endl;
        return -1;
    }
    
    std::unique_ptr<Engine> vulkanTest = std::make_unique<Engine>(root);
    vulkanTest->init();
    vulkanTest->loop();
    
    return 0;
}
