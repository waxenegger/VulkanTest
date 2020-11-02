project('vulkan', 'cpp', default_options : ['cpp_std=c++17', 'werror=true'])

WINDOWS_SDL_PATH = join_paths(meson.source_root(),'SDL2-2.0.12')
WINDOWS_LUNAR_G_PATH = join_paths(meson.source_root(),'VulkanSDK')

includeDir = []
dependencies = []

threadDep = dependency('threads', required: true)

sdlDep = dependency('SDL2', required: false)
vulkanDep = dependency('vulkan')

if sdlDep.found()
   dependencies = [threadDep, sdlDep, vulkanDep]
else
   add_project_arguments('-std:c++latest', language:'cpp')
   sdlIncDir = join_paths(WINDOWS_SDL_PATH, 'include')
   sdlLibDir = join_paths(WINDOWS_SDL_PATH, 'lib')
   sdlDepWin = meson.get_compiler('cpp').find_library('SDL2', dirs: [sdlLibDir])
   sdlDep2Win = meson.get_compiler('cpp').find_library('SDL2main', dirs: [sdlLibDir])
   vulkanIncDir = join_paths(WINDOWS_LUNAR_G_PATH, 'Include')
   vulkanLibDir = join_paths(WINDOWS_LUNAR_G_PATH, 'Lib')
   vulkanDepWin = meson.get_compiler('cpp').find_library('vulkan-1', dirs: [vulkanLibDir])

   dependencies = [threadDep, sdlDepWin, sdlDep2Win, vulkanDepWin]
   includeDir = [sdlIncDir, vulkanIncDir]

endif

src = [ join_paths('src','Test.cpp'), join_paths('src','Graphics.cpp') ]

executable('VulkanTest',  join_paths('src','Test.cpp'), src, include_directories: includeDir, dependencies: dependencies) 
