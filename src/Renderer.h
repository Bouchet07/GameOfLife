// src/Renderer.h
// Manages all OpenGL rendering, including shaders, FBOs, textures,
// view transformations, and simulation logic.

#pragma once

#include <memory>
#include <vector>
#include <utility>
#include <GL/glew.h>
#include "Shader.h"

// Forward declare GLFWwindow to avoid including the full header
struct GLFWwindow;

class Renderer {
public:
    Renderer(GLFWwindow* window, int windowWidth, int windowHeight, int gridWidth, int gridHeight);
    ~Renderer();

    // --- Core Functions ---
    void runSimulationStep();
    void drawToScreen();
    void randomizeBoard();
    void clearBoard();
    void handleMouseDrawing(bool isDrawing, const std::pair<double, double>& mousePos, bool isGliderMode, int gliderRotation);
    void resizeGrid(int newWidth, int newHeight);

    // --- Event Handlers ---
    void onWindowResize(int newWidth, int newHeight);

    // --- View Controls (formerly Camera) ---
    void pan(double screenDx, double screenDy);
    void zoomAt(double screenX, double screenY, float zoomFactor);
    void resetView();

    // --- Coordinate Conversion ---
    std::pair<float, float> screenToTextureCoords(double screenX, double screenY) const;

private:
    GLFWwindow* window;
    
    void initQuad();
    void initTextures();
    void initFramebuffers();
    void drawPattern(int centerX, int centerY, const std::vector<std::pair<int, int>>& pattern, int rotation);

    // Window and Grid dimensions
    int windowWidth;
    int windowHeight;
    int GRID_WIDTH;
    int GRID_HEIGHT;

    // OpenGL resources
    std::unique_ptr<Shader> computeProgram;
    std::unique_ptr<Shader> drawProgram;
    GLuint fbo[2];
    GLuint textures[2];
    int currentTextureIndex = 0;
    GLuint quadVAO, quadVBO;

    // View state
    float panX = 0.0f;
    float panY = 0.0f;
    float zoom = 1.0f;
    float maxZoom;
};