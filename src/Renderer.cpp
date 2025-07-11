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
    zoom = std::max(0.1f, std::min(zoom, 50.0f));

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

std::pair<float, float> Renderer::screenToTextureCoords(double screenX, double screenY) const {
    int winWidthPoints, winHeightPoints;
    glfwGetWindowSize(window, &winWidthPoints, &winHeightPoints);

    if (winWidthPoints == 0 || winHeightPoints == 0) {
        return { -1.f, -1.f };
    }

    // 1. Normalize mouse coordinates to [0, 1], same as v_unscaledUv in the shader
    float normX = (float)screenX / winWidthPoints;
    float normY = 1.0f - ((float)screenY / winHeightPoints);

    // 2. Calculate the scale factor, same as the shader
    float windowAspect = (float)winWidthPoints / (float)winHeightPoints;
    float gridAspect = (float)GRID_WIDTH / (float)GRID_HEIGHT;

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (windowAspect > gridAspect) {
        scaleX = gridAspect / windowAspect;
    }
    else {
        scaleY = windowAspect / gridAspect;
    }

    // 3. THE FIX: Perform the EXACT same transformation as the vertex shader.
    // The previous error was using division here instead of multiplication.
    float correctedX = (normX - 0.5f) * scaleX + 0.5f;
    float correctedY = (normY - 0.5f) * scaleY + 0.5f;

    return { correctedX, correctedY };
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

    auto correctedUv = screenToTextureCoords(mousePos.first, mousePos.second);
    if (correctedUv.first < 0.f || correctedUv.first > 1.f || correctedUv.second < 0.f || correctedUv.second > 1.f) {
        return;
    }

    float finalTexCoordX = correctedUv.first / zoom + panX;
    float finalTexCoordY = correctedUv.second / zoom + panY;

    int gridX = static_cast<int>(finalTexCoordX * GRID_WIDTH);
    int gridY = static_cast<int>(finalTexCoordY * GRID_HEIGHT);

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