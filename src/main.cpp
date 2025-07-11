// src/main.cpp
// The main entry point for the application.
// It creates and runs the GameOfLife application instance.

#include "GameOfLife.h"
#include <iostream>

// --- Configuration ---
// The grid can be any size. The renderer will add letter/pillarboxing to maintain the aspect ratio.
int GRID_WIDTH = 200;
int GRID_HEIGHT = 200;
constexpr int INITIAL_WINDOW_WIDTH = 800;
constexpr int INITIAL_WINDOW_HEIGHT = 800;

int main() {
    try {
        // Create the main application object
        GameOfLife game(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT, GRID_WIDTH, GRID_HEIGHT, "GPU Conway's Game of Life");

        // Start the main loop
        game.run();

    }
    catch (const std::exception& e) {
        // Catch and report any exceptions that occur during initialization or runtime
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
