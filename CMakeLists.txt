cmake_minimum_required(VERSION 3.16)

project(game-of-life)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenMP REQUIRED)

# Create the executable
add_executable(${PROJECT_NAME} "src/game_gpu.cpp")

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
        OpenMP::OpenMP_CXX
)