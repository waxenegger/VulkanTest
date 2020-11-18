
#include "includes/graphics.h"

class Test final {

private:
    const std::string APP_NAME = "Vulkan Test";
    Graphics & GRAPHICS = Graphics::instance();

public:
    void run() {
        this->GRAPHICS.init(this->APP_NAME, VK_MAKE_VERSION(1,0,0));
        this->GRAPHICS.listVkPhysicalDevices();
        this->GRAPHICS.listVkExtensions();
        this->GRAPHICS.listVkLayerProperties();
        auto ret = this->GRAPHICS.getVkPhysicalDeviceQueueFamilyProperties(1);
        this->GRAPHICS.listVkPhysicalDeviceQueueFamilyProperties(ret);
    }
};

int main(int argc, char **argv) {
    Test vulkanTest;
    vulkanTest.run();

    return 0;
}
