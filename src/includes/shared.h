#ifndef SRC_INCLUDES_SHARED_H_
#define SRC_INCLUDES_SHARED_H_

#include <iostream>
#include <fstream>
#include <sstream>

#include <vector>

#include <tuple>

#include <string.h>

#include <SDL.h>
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

#endif
