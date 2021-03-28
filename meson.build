project('vulkan', 'cpp', default_options : ['cpp_std=c++17', 'werror=true'])

WINDOWS_SDL_PATH = join_paths(meson.source_root(),'SDL2-2.0.12')
WINDOWS_SDL_IMAGE_PATH = join_paths(meson.source_root(),'SDL2_image-2.0.5')
WINDOWS_SDL_FONT_PATH = join_paths(meson.source_root(),'SDL2_ttf-2.0.15')
WINDOWS_LUNAR_G_PATH = join_paths(meson.source_root(),'VulkanSDK')
WINDOWS_ASSIMP_PATH = join_paths(meson.source_root(),'assimp')


includeDir = []
dependencies = []

threadDep = dependency('threads', required: true)

sdlDep = dependency('SDL2', required: false)

if sdlDep.found()
    sdlImageDep = dependency('SDL2_image')
    sdlFontDep = dependency('SDL2_ttf')
    vulkanDep = dependency('vulkan')
    glmDep = dependency('glm', required: true)
    assimpDep = dependency('assimp', required: true)
    libDlDep = meson.get_compiler('cpp').find_library('dl', required: true)

    dependencies = [libDlDep, threadDep, glmDep, assimpDep, sdlDep, sdlImageDep, sdlFontDep, vulkanDep]
else
    add_project_arguments('-std:c++latest', language:'cpp')
    
    sdlIncDir = join_paths(WINDOWS_SDL_PATH, 'include')
    sdlLibDir = join_paths(WINDOWS_SDL_PATH, 'lib')
    sdlFontIncDir = join_paths(WINDOWS_SDL_FONT_PATH, 'include')
    sdlFontLibDir = join_paths(WINDOWS_SDL_FONT_PATH, 'lib', 'x64')   
    sdlImageIncDir = join_paths(WINDOWS_SDL_IMAGE_PATH, 'include')
    sdlImageLibDir = join_paths(WINDOWS_SDL_IMAGE_PATH, 'lib', 'x64')
    assimpIncDir = join_paths(WINDOWS_ASSIMP_PATH, 'include')
    assimpLibDir = join_paths(WINDOWS_ASSIMP_PATH, 'lib')
    vulkanIncDir = join_paths(WINDOWS_LUNAR_G_PATH, 'Include')
    vulkanLibDir = join_paths(WINDOWS_LUNAR_G_PATH, 'Lib')

    vulkanDepWin = meson.get_compiler('cpp').find_library('vulkan-1', dirs: [vulkanLibDir])
    assimpDepWin = meson.get_compiler('cpp').find_library('assimp-vc141-mtd', dirs: [assimpLibDir])
    sdlImageDepWin = meson.get_compiler('cpp').find_library('SDL2_image', dirs: [sdlImageLibDir])
    sdlFontDepWin = meson.get_compiler('cpp').find_library('SDL2_ttf', dirs: [sdlFontLibDir])
    sdlDepWin = meson.get_compiler('cpp').find_library('SDL2', dirs: [sdlLibDir])
    sdlDep2Win = meson.get_compiler('cpp').find_library('SDL2main', dirs: [sdlLibDir])

    dependencies = [threadDep, assimpDepWin, sdlDepWin, sdlDep2Win, sdlImageDepWin, sdlFontDepWin, vulkanDepWin]
    includeDir = [sdlIncDir, assimpIncDir, vulkanIncDir, sdlImageIncDir, sdlFontIncDir]
endif

src = [ join_paths('src','Main.cpp'), 
        join_paths('src','Engine.cpp'), 
        join_paths('src','Components.cpp'), 
        join_paths('src','Camera.cpp'), 
        join_paths('src','Graphics.cpp'), 
        join_paths('src','Models.cpp') ]

executable('VulkanTest',  join_paths('src','Main.cpp'), src, include_directories: includeDir, dependencies: dependencies) 
