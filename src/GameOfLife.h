// src/GameOfLife.h
// Main application class. Encapsulates the window, renderer, input handling,
// and the main application loop.

#pragma once

#include <string>
#include <memory>
#include "Renderer.h"
#include "InputHandler.h"

struct GLFWwindow;

class GameOfLife {
public:
    GameOfLife(int windowWidth, int windowHeight, int gridWidth, int gridHeight, const std::string& title);
    ~GameOfLife();
    void run();

private:
    void initWindow(const std::string& title);
    void mainLoop();
    void updateFpsCounter();
    void processInput();
    void onWindowResize(int width, int height);
    void promptAndResizeGrid();
    void promptAndSetSpeed();

    int windowWidth;
    int windowHeight;
    int gridWidth;
    int gridHeight;

    GLFWwindow* window;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputHandler> inputHandler;

    bool isPaused = false;

    double lastTime = 0.0;
    int frameCount = 0;

    bool limitSpeed = true;
    double updatesPerSecond = 30.0;
    double timeOfLastUpdate = 0.0;
};