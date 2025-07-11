// src/InputHandler.cpp
// Implementation of the InputHandler class.

#include "InputHandler.h"
#include "Renderer.h"
#include <GLFW/glfw3.h>
#include <iostream>

InputHandler::InputHandler(GLFWwindow* win, Renderer& rend)
    : window(win), renderer(rend) {
    glfwSetWindowUserPointer(window, this);
}

void InputHandler::setupCallbacks() {
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
}

bool InputHandler::wasKeyPressed(int key) {
    if (keys[key] && !keysProcessed[key]) {
        keysProcessed[key] = true;
        return true;
    }
    return false;
}

void InputHandler::toggleGliderMode() {
    isGliderMode = !isGliderMode;
    std::cout << "Glider mode " << (isGliderMode ? "ON" : "OFF") << std::endl;
}

void InputHandler::rotateGlider() {
    if (isGliderMode) {
        gliderRotation = (gliderRotation + 1) % 4;
        std::cout << "Glider rotation set to " << gliderRotation * 90 << " degrees." << std::endl;
    }
}

// --- Static Callback Implementations ---

void InputHandler::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    InputHandler* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    if (!handler) return;

    if (action == GLFW_PRESS) {
        handler->keys[key] = true;
        handler->keysProcessed[key] = false;

        switch (key) {
        case GLFW_KEY_L:
            handler->isFpsLimited = !handler->isFpsLimited;
            glfwSwapInterval(handler->isFpsLimited ? 1 : 0);
            std::cout << "FPS limit " << (handler->isFpsLimited ? "ON (V-Sync)" : "OFF") << std::endl;
            break;
        case GLFW_KEY_H:
            handler->renderer.resetView();
            std::cout << "View reset." << std::endl;
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, true);
            break;
        }
    }
    else if (action == GLFW_RELEASE) {
        handler->keys[key] = false;
        handler->keysProcessed[key] = false;
    }
}

void InputHandler::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    InputHandler* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    if (!handler) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        // This now handles both pressing and releasing the button
        if (action == GLFW_PRESS) {
            handler->isLeftMouseDrag = true;
        }
        else if (action == GLFW_RELEASE) {
            handler->isLeftMouseDrag = false;
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        handler->isRightMouseButtonPressed = (action == GLFW_PRESS);
        if (handler->isRightMouseButtonPressed) {
            glfwGetCursorPos(window, &handler->lastMouseX, &handler->lastMouseY);
        }
    }
}

void InputHandler::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    InputHandler* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    if (!handler) return;

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    handler->renderer.zoomAt(mouseX, mouseY, yoffset > 0 ? 1.1f : 1.0f / 1.1f);
}

void InputHandler::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    InputHandler* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    if (!handler) return;

    handler->mousePosition = { xpos, ypos };

    if (handler->isRightMouseButtonPressed) {
        double dx = xpos - handler->lastMouseX;
        double dy = ypos - handler->lastMouseY;
        handler->renderer.pan(dx, dy);
    }

    handler->lastMouseX = xpos;
    handler->lastMouseY = ypos;
}

void InputHandler::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // This now correctly gets the InputHandler pointer
    auto* handler = static_cast<InputHandler*>(glfwGetWindowUserPointer(window));
    if (handler) {
        // The handler has a reference to the renderer, so it can call this
        handler->renderer.onWindowResize(width, height);
    }
}