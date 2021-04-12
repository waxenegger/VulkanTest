#ifndef SRC_INCLUDES_SHARED_INCL_H_
#define SRC_INCLUDES_SHARED_INCL_H_

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>
#include <array>
#include <map>
#include <functional>
#include <algorithm>
#include <queue>
#include <mutex>
#include <condition_variable>

#include <tuple>

#include <thread>
#include <memory>
#include <chrono>

#include <string.h>
#include <filesystem>

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define ASSERT_VULKAN(val) \
    if (val != VK_SUCCESS) { \
        std::cerr << "Error: " << val << std::endl; \
    };

    
static constexpr bool DEBUG_BBOX = false;
static constexpr uint32_t MEGA_BYTE = 1000 * 1000;

#endif
