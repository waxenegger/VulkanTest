
#include "includes/graphics.h"

class Test final {

private:
    const std::string APP_NAME = "Vulkan Test";
    Graphics & graphics = Graphics::instance();

public:
    void run() {
        this->graphics.init(this->APP_NAME, VK_MAKE_VERSION(1,0,0));
    }
};

int main(int argc, char **argv) {
    Test vulkanTest;
    vulkanTest.run();



    return 0;
}
