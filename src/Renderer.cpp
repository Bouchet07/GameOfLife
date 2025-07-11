// src/Renderer.cpp
// Implementation of the Renderer class.

#include "Renderer.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <random>
#include <iostream>
#include <algorithm>

Renderer::Renderer(GLFWwindow* win, int winWidth, int winHeight, int gridW, int gridH)
    : window(win),
    windowWidth(winWidth),
    windowHeight(winHeight),
    GRID_WIDTH(gridW),
    GRID_HEIGHT(gridH) {

    computeProgram = std::make_unique<Shader>("shaders/compute.vert", "shaders/compute.frag");
    drawProgram = std::make_unique<Shader>("shaders/draw.vert", "shaders/draw.frag");

    initQuad();
    initTextures();
    initFramebuffers();
    resetView();

    // Calculate the initial max zoom level
    float maxDimension = std::max(GRID_WIDTH, GRID_HEIGHT);
    maxZoom = maxDimension / 10.0f; // Allow zooming to see a minimum of 10 cells
}

Renderer::~Renderer() {
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteFramebuffers(2, fbo);
    glDeleteTextures(2, textures);
}

void Renderer::onWindowResize(int newWidth, int newHeight) {
    windowWidth = newWidth;
    windowHeight = newHeight;
}

void Renderer::resetView() {
    panX = 0.0f;
    panY = 0.0f;
    zoom = 1.0f;
}

void Renderer::pan(double screenDx, double screenDy) {
    panX -= (float)(screenDx / (windowWidth * zoom));
    panY += (float)(screenDy / (windowHeight * zoom));
}

void Renderer::zoomAt(double screenX, double screenY, float zoomFactor) {
    double invertedY = windowHeight - screenY;

    float texX_before = (float)(screenX / windowWidth) / zoom + panX;
    float texY_before = (float)(invertedY / windowHeight) / zoom + panY;

    zoom *= zoomFactor;
    zoom = std::max(0.1f, std::min(zoom, this->maxZoom));

    float texX_after = (float)(screenX / windowWidth) / zoom + panX;
    float texY_after = (float)(invertedY / windowHeight) / zoom + panY;

    panX += texX_before - texX_after;
    panY += texY_before - texY_after;
}

void Renderer::initQuad() {
    float quadVertices[] = { -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f, -1.0f };
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void Renderer::initTextures() {
    glGenTextures(2, textures);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, GRID_WIDTH, GRID_HEIGHT, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::initFramebuffers() {
    glGenFramebuffers(2, fbo);
    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[i], 0);
    }
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Framebuffer is not complete!");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Helper for linear interpolation (the C++ version of GLSL's mix function)
float mix(float a, float b, float t) {
    return a * (1.0f - t) + b * t;
}

std::pair<float, float> Renderer::screenToTextureCoords(double screenX, double screenY) const {
    // Normalize mouse coordinates to [-1, 1] range, with Y inverted.
    float normX = (float)(screenX / windowWidth) * 2.0f - 1.0f;
    float normY = 1.0f - (float)(screenY / windowHeight) * 2.0f;

    // Re-calculate the aspect ratio scaling exactly as in the vertex shader.
    float windowAspect = (float)windowWidth / (float)windowHeight;
    float gridAspect = (float)GRID_WIDTH / (float)GRID_HEIGHT;
    float scaleX = 1.0f, scaleY = 1.0f;
    if (windowAspect > gridAspect) {
        scaleX = gridAspect / windowAspect;
    }
    else {
        scaleY = windowAspect / gridAspect;
    }

    // Reverse the scaling to find the position on the unscaled quad.
    // If the click is outside this scaled area (in the letterbox), we return an invalid coordinate.
    if (abs(normX) > abs(scaleX) || abs(normY) > abs(scaleY)) {
        return { -1.0f, -1.0f };
    }
    float unscaledX = normX / scaleX;
    float unscaledY = normY / scaleY;

    // Now, reverse the pan and zoom transformation from the vertex shader.
    // Convert from [-1, 1] to [0, 1] UV range.
    float quad_uv_x = (unscaledX + 1.0f) / 2.0f;
    float quad_uv_y = (unscaledY + 1.0f) / 2.0f;

    // Finally, apply the zoom and pan to get the final texture coordinate.
    float texCoordX = quad_uv_x / zoom + panX;
    float texCoordY = quad_uv_y / zoom + panY;

    return { texCoordX, texCoordY };
}

void Renderer::randomizeBoard() {
    std::vector<float> data(GRID_WIDTH * GRID_HEIGHT);
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> distrib(0, 1);
    for (auto& cell : data) {
        cell = static_cast<float>(distrib(gen));
    }
    glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_WIDTH, GRID_HEIGHT, GL_RED, GL_FLOAT, data.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    std::cout << "Board randomized." << std::endl;
}

void Renderer::clearBoard() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo[currentTextureIndex]);
    glViewport(0, 0, GRID_WIDTH, GRID_HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::cout << "Board cleared." << std::endl;
}

void Renderer::runSimulationStep() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo[1 - currentTextureIndex]);
    glViewport(0, 0, GRID_WIDTH, GRID_HEIGHT);
    computeProgram->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);
    computeProgram->setInt("u_currentState", 0);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    currentTextureIndex = 1 - currentTextureIndex;
}

void Renderer::drawToScreen() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    drawProgram->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);
    drawProgram->setInt("u_screenTexture", 0);
    drawProgram->setVec2("u_pan", panX, panY);
    drawProgram->setFloat("u_zoom", zoom);
    drawProgram->setVec2("u_resolution", (float)windowWidth, (float)windowHeight);
    drawProgram->setVec2("u_gridResolution", (float)GRID_WIDTH, (float)GRID_HEIGHT);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Renderer::handleMouseDrawing(bool isLeftMouseDrag, const std::pair<double, double>& mousePos, bool isGliderMode, int gliderRotation) {
    if (!isLeftMouseDrag) return;

    // THIS FUNCTION IS NOW SIMPLER
    // We get the final texture coordinate directly from our corrected conversion function.
    auto texCoords = screenToTextureCoords(mousePos.first, mousePos.second);

    if (texCoords.first < 0.f || texCoords.first > 1.f || texCoords.second < 0.f || texCoords.second > 1.f) {
        return; // Click was outside the valid grid area
    }

    int gridX = static_cast<int>(texCoords.first * GRID_WIDTH);
    int gridY = static_cast<int>(texCoords.second * GRID_HEIGHT);

    if (isGliderMode) {
        static const std::vector<std::pair<int, int>> gliderPattern = { {1, 0}, {2, 1}, {0, 2}, {1, 2}, {2, 2} };
        drawPattern(gridX, gridY, gliderPattern, gliderRotation);
    }
    else {
        if (gridX >= 0 && gridX < GRID_WIDTH && gridY >= 0 && gridY < GRID_HEIGHT) {
            float white = 1.0f;
            glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);
            glTexSubImage2D(GL_TEXTURE_2D, 0, gridX, gridY, 1, 1, GL_RED, GL_FLOAT, &white);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

void Renderer::drawPattern(int centerX, int centerY, const std::vector<std::pair<int, int>>& pattern, int rotation) {
    float white = 1.0f;
    glBindTexture(GL_TEXTURE_2D, textures[currentTextureIndex]);
    for (const auto& p : pattern) {
        int dx = p.first, dy = p.second;
        int rot_dx = dx, rot_dy = dy;
        switch (rotation) {
        case 1: rot_dx = -dy; rot_dy = dx; break;
        case 2: rot_dx = -dx; rot_dy = -dy; break;
        case 3: rot_dx = dy; rot_dy = -dx; break;
        }
        int finalX = centerX + rot_dx;
        int finalY = centerY + rot_dy;
        if (finalX >= 0 && finalX < GRID_WIDTH && finalY >= 0 && finalY < GRID_HEIGHT) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, finalX, finalY, 1, 1, GL_RED, GL_FLOAT, &white);
        }
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::resizeGrid(int newWidth, int newHeight) {
    // 1. Update the stored dimensions
    GRID_WIDTH = newWidth;
    GRID_HEIGHT = newHeight;
    std::cout << "Resizing grid to " << GRID_WIDTH << "x" << GRID_HEIGHT << std::endl;

    // 2. Clean up the old OpenGL objects that depend on grid size
    glDeleteFramebuffers(2, fbo);
    glDeleteTextures(2, textures);

    // 3. Re-create the textures and framebuffers with the new size
    //    (Our existing init functions work perfectly for this)
    initTextures();
    initFramebuffers();

    // 4. Initialize the new grid with a fresh state
    randomizeBoard();

    // Recalculate max zoom for the new grid size
    float maxDimension = std::max(newWidth, newHeight);
    maxZoom = maxDimension / 10.0f;

    resetView();
}