// src/InputHandler.h
// Manages all GLFW input callbacks and state.

#pragma once

#include <utility>

// Forward declarations to avoid including full headers
struct GLFWwindow;
class Renderer;

class InputHandler {
public:
    InputHandler(GLFWwindow* win, Renderer& renderer);

    void setupCallbacks();

    // State query methods
    bool wasKeyPressed(int key);
    bool getMouseDragState() const { return isLeftMouseDrag; }
    const std::pair<double, double>& getMousePosition() const { return mousePosition; }

    // Glider mode methods
    void toggleGliderMode();
    void rotateGlider();
    bool isGliderModeActive() const { return isGliderMode; }
    int getGliderRotation() const { return gliderRotation; }

private:
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    GLFWwindow* window;
    Renderer& renderer;

    bool keys[1024] = { false };
    bool keysProcessed[1024] = { false };
    bool isLeftMouseDrag = false;
    bool isRightMouseButtonPressed = false;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    std::pair<double, double> mousePosition;
    bool isFpsLimited = true;

    bool isGliderMode = false;
    int gliderRotation = 0;
};