// Conway's Game of Life - Enhanced GPU Version
//
// Features:
// - GPU-accelerated simulation using OpenGL shaders.
// - Arbitrary grid resolution with correct aspect ratio rendering.
// - Interactive camera: Zoom (mouse wheel) and Pan (right-click drag).
// - Pause/Resume simulation with SPACE.
// - Draw new cells with Left-click drag.
// - Clear the board with 'C'.
// - Re-randomize the board with 'R'.
// - Reset view (pan/zoom) with 'H'.
// - FPS counter in the window title.
// - Toggleable FPS limit (V-Sync) with 'L'.
// - Glider drawing mode with 'G', rotate glider with 'T'.
// - Correctly handles window resizing.
//
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <string>
#include <random>
#include <algorithm> // For std::max/min

// --- Configuration ---
// The grid can be any size. The renderer will add letter/pillarboxing to maintain the aspect ratio.
constexpr int GRID_WIDTH = 1920*3;
constexpr int GRID_HEIGHT = 1080*3;

// --- Globals ---
GLFWwindow* window;
// Use variables for window dimensions to handle resizing
int windowWidth = 1920;
int windowHeight = 1080;

// Shader Programs
GLuint computeProgram;
GLuint drawProgram;

// Framebuffer Objects for ping-ponging textures
GLuint fbo[2];
GLuint textures[2];
int currentTextureIndex = 0;

// Fullscreen Quad resources
GLuint quadVAO, quadVBO;

// Interaction State
bool isPaused = false;
bool isLeftMouseDrag = false;
bool isRightMouseButtonPressed = false;
double lastMouseX = 0.0, lastMouseY = 0.0;
bool isFpsLimited = true;

// Glider Mode State
bool isGliderMode = false;
int gliderRotation = 0; // 0: 0 deg, 1: 90 deg, 2: 180 deg, 3: 270 deg

// View/Camera State
struct Camera {
    float panX = 0.0f;
    float panY = 0.0f;
    float zoom = 1.0f;
} camera;


// --- Shader Sources ---

// Vertex shader for drawing. It calculates aspect-ratio-correct texture coordinates.
const char* drawVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;

out vec2 v_texCoord;
out vec2 v_unscaledUv; // Pass this to fragment shader for bounds checking

uniform vec2 u_pan;
uniform float u_zoom;
uniform vec2 u_resolution;
uniform vec2 u_gridResolution;

void main() {
    v_unscaledUv = (aPos + 1.0) / 2.0; // This is the [0,1] coordinate of the screen pixel
    vec2 uv = v_unscaledUv;

    float windowAspect = u_resolution.x / u_resolution.y;
    float gridAspect = u_gridResolution.x / u_gridResolution.y;
    
    vec2 scale;
    if (windowAspect > gridAspect) {
        // Window is wider than grid (pillarbox)
        scale = vec2(gridAspect / windowAspect, 1.0);
    } else {
        // Window is taller than grid (letterbox)
        scale = vec2(1.0, windowAspect / gridAspect);
    }
    
    // Scale the coordinate space, then center it
    uv = (uv - 0.5) * scale + 0.5;
    
    // Apply pan and zoom to the corrected coordinates
    v_texCoord = uv / u_zoom + u_pan;
    gl_Position = vec4(aPos, 0.0, 1.0);
})";

// Fragment shader for drawing. It draws the game texture or the letterbox/pillarbox bars.
const char* drawFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 v_texCoord;
in vec2 v_unscaledUv; // Use this for bounds checking

uniform sampler2D u_screenTexture;
uniform vec2 u_resolution;
uniform vec2 u_gridResolution;

void main() {
    float windowAspect = u_resolution.x / u_resolution.y;
    float gridAspect = u_gridResolution.x / u_gridResolution.y;
    
    vec2 scale;
    if(windowAspect > gridAspect) {
        scale = vec2(gridAspect / windowAspect, 1.0);
    } else {
        scale = vec2(1.0, windowAspect / gridAspect);
    }

    // Check if the original screen UV is outside the scaled area
    if (abs(v_unscaledUv.x - 0.5) > scale.x * 0.5 || abs(v_unscaledUv.y - 0.5) > scale.y * 0.5) {
        FragColor = vec4(0.05, 0.05, 0.05, 1.0); // Dark grey for bars
    } else {
         // Use the corrected, panned, and zoomed texture coordinates
         float cellState = texture(u_screenTexture, v_texCoord).r;
         FragColor = vec4(vec3(cellState), 1.0);
    }
})";

// Passthrough vertex shader for the computation step.
const char* computeVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 v_texCoord;
void main() {
    v_texCoord = (aPos + 1.0) / 2.0;
    gl_Position = vec4(aPos, 0.0, 1.0);
})";

// The core Game of Life logic, executed entirely on the GPU.
const char* computeFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 v_texCoord;
uniform sampler2D u_currentState;
float getCellState(vec2 coord) {
    return texture(u_currentState, coord).r;
}
void main() {
    vec2 pixel = 1.0 / textureSize(u_currentState, 0);
    int aliveNeighbors = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            aliveNeighbors += int(getCellState(v_texCoord + vec2(float(dx), float(dy)) * pixel));
        }
    }
    float currentState = getCellState(v_texCoord);
    float newState = 0.0;
    if (currentState > 0.5 && (aliveNeighbors == 2 || aliveNeighbors == 3)) {
        newState = 1.0;
    } else if (currentState < 0.5 && aliveNeighbors == 3) {
        newState = 1.0;
    }
    FragColor = vec4(vec3(newState), 1.0);
})";


// --- Utility Functions ---

void checkShaderError(GLuint shader, const std::string& type) {
    GLint success; GLchar infoLog[1024];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(shader, 1024, NULL, infoLog); std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n"; }
}

void checkProgramError(GLuint program) {
    GLint success; GLchar infoLog[1024];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) { glGetProgramInfoLog(program, 1024, NULL, infoLog); std::cerr << "ERROR::PROGRAM_LINKING_ERROR\n" << infoLog << "\n"; }
}

GLuint createShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    checkShaderError(shader, type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
    return shader;
}

GLuint createProgram(const char* vsSrc, const char* fsSrc) {
    GLuint vs = createShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = createShader(GL_FRAGMENT_SHADER, fsSrc);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs); glAttachShader(program, fs);
    glLinkProgram(program);
    checkProgramError(program);
    glDeleteShader(vs); glDeleteShader(fs);
    return program;
}

// --- Initialization ---

void initQuad() {
    float quadVertices[] = { -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f };
    glGenVertexArrays(1, &quadVAO); glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
}

void initTextures() {
    glGenTextures(2, textures);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, GRID_WIDTH, GRID_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
}

void randomizeBoard() {
    std::vector<float> data(GRID_WIDTH * GRID_HEIGHT);
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> distrib(0, 1);
    for (auto& cell : data) { cell = static_cast<float>(distrib(gen)); }
    glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RED, GL_FLOAT, data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    std::cout << "Board randomized." << std::endl;
}

void clearBoard() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo[currentTextureIndex]);
    glViewport(0, 0, GRID_WIDTH, GRID_HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "Board cleared." << std::endl;
}

void initFramebuffers() {
    glGenFramebuffers(2, fbo);
    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);
    }
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// --- GLFW Callbacks ---

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Update window dimensions and viewport on resize
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_SPACE: isPaused = !isPaused; std::cout << (isPaused ? "Paused" : "Resumed") << std::endl; break;
        case GLFW_KEY_C: clearBoard(); break;
        case GLFW_KEY_R: randomizeBoard(); break;
        case GLFW_KEY_L:
            isFpsLimited = !isFpsLimited;
            glfwSwapInterval(isFpsLimited ? 1 : 0);
            std::cout << "FPS limit " << (isFpsLimited ? "ON (V-Sync)" : "OFF") << std::endl;
            break;
        case GLFW_KEY_G:
            isGliderMode = !isGliderMode;
            std::cout << "Glider mode " << (isGliderMode ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_T:
            if (isGliderMode) {
                gliderRotation = (gliderRotation + 1) % 4;
                std::cout << "Glider rotation set to " << gliderRotation * 90 << " degrees." << std::endl;
            }
            break;
        case GLFW_KEY_H:
            camera.panX = 0.0f;
            camera.panY = 0.0f;
            camera.zoom = 1.0f;
            std::cout << "View reset." << std::endl;
            break;
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, true); break;
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        isLeftMouseDrag = (action == GLFW_PRESS);
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        isRightMouseButtonPressed = (action == GLFW_PRESS);
        if (isRightMouseButtonPressed) { glfwGetCursorPos(window, &lastMouseX, &lastMouseY); }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    double mouseX, mouseY; glfwGetCursorPos(window, &mouseX, &mouseY);
    float texX_before = (float)(mouseX / windowWidth) / camera.zoom + camera.panX;
    float texY_before = (float)((windowHeight - mouseY) / windowHeight) / camera.zoom + camera.panY;
    if (yoffset > 0) camera.zoom *= 1.1f; else camera.zoom /= 1.1f;
    camera.zoom = std::max(0.1f, std::min(camera.zoom, 50.0f));
    float texX_after = (float)(mouseX / windowWidth) / camera.zoom + camera.panX;
    float texY_after = (float)((windowHeight - mouseY) / windowHeight) / camera.zoom + camera.panY;
    camera.panX += texX_before - texX_after;
    camera.panY += texY_before - texY_after;
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (isRightMouseButtonPressed) {
        double dx = xpos - lastMouseX; double dy = ypos - lastMouseY;
        camera.panX -= (float)(dx / (windowWidth * camera.zoom));
        camera.panY += (float)(dy / (windowHeight * camera.zoom));
        lastMouseX = xpos; lastMouseY = ypos;
    }
}

// --- Main Logic ---

void drawPattern(int centerX, int centerY, const std::vector<std::pair<int, int>>& pattern) {
    float white = 1.0f;
    glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);

    for (const auto& p : pattern) {
        int dx = p.first;
        int dy = p.second;

        // Apply rotation
        int rot_dx = dx, rot_dy = dy;
        switch (gliderRotation) {
        case 1: rot_dx = -dy; rot_dy = dx; break; // 90 degrees
        case 2: rot_dx = -dx; rot_dy = -dy; break; // 180 degrees
        case 3: rot_dx = dy; rot_dy = -dx; break; // 270 degrees
        }

        int finalX = centerX + rot_dx;
        int finalY = centerY + rot_dy;

        if (finalX >= 0 && finalX < GRID_WIDTH && finalY >= 0 && finalY < GRID_HEIGHT) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, finalX, finalY, 1, 1, GL_RED, GL_FLOAT, &white);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void handleMouseInput() {
    if (!isLeftMouseDrag) return;

    double mouseX, mouseY; glfwGetCursorPos(window, &mouseX, &mouseY);
    float texCoordX = (float)(mouseX / windowWidth) / camera.zoom + camera.panX;
    float texCoordY = (float)((windowHeight - mouseY) / windowHeight) / camera.zoom + camera.panY;
    int gridX = static_cast<int>(texCoordX * GRID_WIDTH);
    int gridY = static_cast<int>(texCoordY * GRID_HEIGHT);

    if (isGliderMode) {
        // The standard "Glider" pattern relative to a center point.
        static const std::vector<std::pair<int, int>> gliderPattern = {
            {1, 0}, {2, 1}, {0, 2}, {1, 2}, {2, 2}
        };
        drawPattern(gridX, gridY, gliderPattern);
        isLeftMouseDrag = false; // Prevent dragging multiple gliders
    }
    else {
        // Original drawing logic for single cells
        if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
            float white = 1.0f;
            glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, gridX, gridY, 1, 1, GL_RED, GL_FLOAT, &white);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

void runSimulationStep() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo[1 - currentTextureIndex]);
    glViewport(0, 0, GRID_WIDTH, GRID_HEIGHT);
    glUseProgram(computeProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);
    glUniform1i(glGetUniformLocation(computeProgram, "u_currentState"), 0);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    currentTextureIndex = 1 - currentTextureIndex;
}

void drawToScreen() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(drawProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);
    glUniform1i(glGetUniformLocation(drawProgram, "u_screenTexture"), 0);
    glUniform2f(glGetUniformLocation(drawProgram, "u_pan"), camera.panX, camera.panY);
    glUniform1f(glGetUniformLocation(drawProgram, "u_zoom"), camera.zoom);
    // Pass resolution uniforms for aspect ratio correction
    glUniform2f(glGetUniformLocation(drawProgram, "u_resolution"), (float)windowWidth, (float)windowHeight);
    glUniform2f(glGetUniformLocation(drawProgram, "u_gridResolution"), (float)GRID_WIDTH, (float)GRID_HEIGHT);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void mainLoop() {
    double lastTime = glfwGetTime(); int frameCount = 0;
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime(); frameCount++;
        if (currentTime - lastTime >= 1.0) {
            std::string title = "GPU Conway's Game of Life | FPS: " + std::to_string(frameCount) + " | " + (isPaused ? "Paused" : "Running");
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0; lastTime = currentTime;
        }
        glfwPollEvents();
        handleMouseInput();
        if (!isPaused) { runSimulationStep(); }
        drawToScreen();
        glfwSwapBuffers(window);
    }
}

void cleanup() {
    glDeleteVertexArrays(1, &quadVAO); glDeleteBuffers(1, &quadVBO);
    glDeleteFramebuffers(2, fbo); glDeleteTextures(2, textures);
    glDeleteProgram(computeProgram); glDeleteProgram(drawProgram);
    glfwTerminate();
}

int main() {
    if (!glfwInit()) { std::cerr << "Failed to initialize GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, "GPU Conway's Game of Life", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create GLFW window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Start with V-Sync ON

    if (glewInit() != GLEW_OK) { std::cerr << "Failed to initialize GLEW\n"; return -1; }

    // --- Set Callbacks ---
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // --- Print Controls ---
    std::cout << "--- Controls ---\n"
        << "SPACE: Pause/Resume\n"
        << "Left Mouse: Draw cells (or place glider)\n"
        << "Right Mouse Drag: Pan view\n"
        << "Mouse Wheel: Zoom view\n"
        << "C: Clear board\n"
        << "R: Randomize board\n"
        << "H: Reset View (Home)\n"
        << "L: Toggle FPS Limit (V-Sync)\n"
        << "G: Toggle Glider Mode\n"
        << "T: Rotate Glider (in Glider Mode)\n"
        << "ESC: Exit\n"
        << "----------------\n";

    // --- OpenGL Object Initialization ---
    computeProgram = createProgram(computeVertexShaderSource, computeFragmentShaderSource);
    drawProgram = createProgram(drawVertexShaderSource, drawFragmentShaderSource);
    initQuad(); initTextures(); initFramebuffers();
    randomizeBoard();

    mainLoop();
    cleanup();
    return 0;
}