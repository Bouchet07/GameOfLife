// src/GameOfLife.cpp
// Implementation of the main GameOfLife application class.

#include "GameOfLife.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <limits>

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
        << "N: New grid with custom dimensions\n"
        << "H: Reset View (Home)\n"
        << "L: Toggle FPS Limit (V-Sync)\n"
        << "K: Toggle Simulation Speed Limit\n"
        << "Up/Down Arrows: Increase/Decrease Sim Speed\n"
        << "S: Set Specific Sim Speed\n"
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
    timeOfLastUpdate = lastTime; // Initialize the last update time

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        processInput();

        double currentTime = glfwGetTime();

        // --- This is the new speed control logic ---
        // Only run a simulation step if the game is not paused AND
        // if enough time has passed since the last update.
        if (!isPaused) {
            if (!limitSpeed || (currentTime - timeOfLastUpdate >= 1.0 / updatesPerSecond)) {
                renderer->runSimulationStep();
                timeOfLastUpdate = currentTime; // Record the time of this update
            }
        }

        // Drawing to the screen still happens every frame
        renderer->drawToScreen();
        glfwSwapBuffers(window);

        // The FPS counter is now separate from the simulation speed
        updateFpsCounter();
    }
}

void GameOfLife::promptAndResizeGrid() {
    isPaused = true; // Pause the simulation while we get input
    std::cout << "\n--- Grid Resize ---\nSimulation paused. Please enter new grid dimensions in the console.\n"
        << "(width height): ";

    int newWidth = 0, newHeight = 0;
    std::cin >> newWidth >> newHeight;

    if (std::cin.good() && newWidth > 0 && newHeight > 0) {
        // THE FIX: Update the GameOfLife object's own grid dimension variables.
        this->gridWidth = newWidth;
        this->gridHeight = newHeight;

        // Now, command the renderer to perform the resize.
        renderer->resizeGrid(newWidth, newHeight);
        std::cout << "Grid resized. Press Space to resume simulation." << std::endl;
    }
    else {
        std::cout << "Invalid input. Please enter two positive numbers." << std::endl;
        // Clear error flags and ignore the rest of the bad input line
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void GameOfLife::promptAndSetSpeed() {
    isPaused = true; // Pause while we get input
    std::cout << "\n--- Set Speed ---\nSimulation paused. Please enter a new speed in the console.\n"
        << "(Updates Per Second): ";

    double newSpeed = 0.0;
    std::cin >> newSpeed;

    // Validate the user's input
    if (std::cin.good() && newSpeed > 0) {
        updatesPerSecond = newSpeed;
        limitSpeed = true; // Re-enable the limit to use the new speed
        std::cout << "Simulation speed set to " << updatesPerSecond << " UPS. Press Space to resume simulation" << std::endl;
    }
    else {
        std::cout << "Invalid input. Please enter a positive number." << std::endl;
        // Clear error flags and ignore the rest of the bad input line
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void GameOfLife::processInput() {
    // Check for speed control keys
    if (inputHandler->wasKeyPressed(GLFW_KEY_K)) {
        limitSpeed = !limitSpeed;
        std::cout << "Simulation speed limit " << (limitSpeed ? "ON" : "OFF") << std::endl;
    }

    if (inputHandler->wasKeyPressed(GLFW_KEY_UP)) {
        updatesPerSecond *= 1.5;
        std::cout << "Simulation speed set to " << updatesPerSecond << " UPS" << std::endl;
    }
    if (inputHandler->wasKeyPressed(GLFW_KEY_DOWN)) {
        updatesPerSecond /= 1.5;
        std::cout << "Simulation speed set to " << updatesPerSecond << " UPS" << std::endl;
    }
    if (inputHandler->wasKeyPressed(GLFW_KEY_S)) {
        promptAndSetSpeed();
    }
    if (inputHandler->wasKeyPressed(GLFW_KEY_N)) {
        promptAndResizeGrid();
    }
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