cmake_minimum_required(VERSION 3.16)

project(game-of-life)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Force static linking for the executable
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

# Detect source files
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Include all necessary header directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        src
        ${GLFW_INCLUDE_DIRS}
)

# Link against the libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenGL::GL
        GLEW::GLEW
        glfw
)

# Tell CMake where to find shader files
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})