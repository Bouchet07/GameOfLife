// src/GameOfLife.cpp
// Implementation of the main GameOfLife application class.

#include "GameOfLife.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

GameOfLife::GameOfLife(int winWidth, int winHeight, int grdWidth, int grdHeight, const std::string& title)
    : windowWidth(winWidth), windowHeight(winHeight), gridWidth(grdWidth), gridHeight(grdHeight), window(nullptr) {

    initWindow(title);

    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    renderer = std::make_unique<Renderer>(window, windowWidth, windowHeight, gridWidth, gridHeight);
    inputHandler = std::make_unique<InputHandler>(window, *renderer);

    inputHandler->setupCallbacks();

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
}

GameOfLife::~GameOfLife() {
    if (window) {
        glfwDestroyWindow(window);
    }
    glfwTerminate();
}

void GameOfLife::initWindow(const std::string& title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(windowWidth, windowHeight, title.c_str(), nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
}

void GameOfLife::onWindowResize(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    renderer->onWindowResize(width, height);
}


void GameOfLife::run() {
    renderer->randomizeBoard();
    mainLoop();
}

void GameOfLife::mainLoop() {
    lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        processInput();
        if (!isPaused) {
            renderer->runSimulationStep();
        }
        renderer->drawToScreen();
        glfwSwapBuffers(window);
        updateFpsCounter();
    }
}

void GameOfLife::processInput() {
    if (inputHandler->wasKeyPressed(GLFW_KEY_SPACE)) {
        isPaused = !isPaused;
        std::cout << (isPaused ? "Paused" : "Resumed") << std::endl;
    }
    if (inputHandler->wasKeyPressed(GLFW_KEY_C)) renderer->clearBoard();
    if (inputHandler->wasKeyPressed(GLFW_KEY_R)) renderer->randomizeBoard();
    if (inputHandler->wasKeyPressed(GLFW_KEY_G)) inputHandler->toggleGliderMode();
    if (inputHandler->wasKeyPressed(GLFW_KEY_T)) inputHandler->rotateGlider();

    renderer->handleMouseDrawing(inputHandler->getMouseDragState(),
        inputHandler->getMousePosition(),
        inputHandler->isGliderModeActive(),
        inputHandler->getGliderRotation());
}

void GameOfLife::updateFpsCounter() {
    double currentTime = glfwGetTime();
    frameCount++;
    if (currentTime - lastTime >= 1.0) {
        std::string title = "GPU Conway's Game of Life | FPS: " + std::to_string(frameCount) + " | " + (isPaused ? "Paused" : "Running");
        glfwSetWindowTitle(window, title.c_str());
        frameCount = 0;
        lastTime = currentTime;
    }
}