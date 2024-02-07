cmake_minimum_required(VERSION 3.26)
include(utils.cmake)

project(
    LearnWebGPU
    VERSION 1.0
    LANGUAGES CXX C
)

if (NOT EMSCRIPTEN)
    # Do not include this with emscripten, it provides its own version.
    add_subdirectory(glfw)
endif()
add_subdirectory(webgpu)
add_subdirectory(glfw3webgpu)

add_executable(App 
    main.cpp 
    webgpu-utils.h 
    webgpu-utils.cpp 
)

set_target_properties(App PROPERTIES
    CXX_STANDARD 20
    VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
    # CXX_EXTENSIONS OFF
    # COMPILE_WARNING_AS_ERROR ON
)


target_link_libraries(App PRIVATE glfw webgpu glfw3webgpu)

# target_treat_all_warnings_as_errors(App)

# At the end of the CMakeLists.txt
if (EMSCRIPTEN)
	# Add Emscripten-specific link options
	target_link_options(App PRIVATE
		-sUSE_GLFW=3 # Use Emscripten-provided GLFW
		-sUSE_WEBGPU # Handle WebGPU symbols
		-sASYNCIFY # Required by WebGPU-C++
		-sALLOW_MEMORY_GROWTH
	)

	# Generate a full web page rather than a simple WebAssembly module
	set_target_properties(App PROPERTIES SUFFIX ".html")
endif()