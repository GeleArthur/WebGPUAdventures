cmake_minimum_required(VERSION 3.26)

project(
    LearnWebGPU
    VERSION 1.0
    LANGUAGES CXX
)

add_executable(App 
main.cpp 
webgpu-utils.h 
webgpu-utils.cpp 
tests.cpp
)

set_target_properties(App PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)

if (MSVC)
    target_compile_options(App PRIVATE /W4)
else()
    target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

add_subdirectory(glfw)
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)
target_copy_webgpu_binaries(App)
