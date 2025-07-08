#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <random>

// --- Configuration ---
constexpr size_t GRID_WIDTH = 100;
constexpr size_t GRID_HEIGHT = 100;
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 800;
constexpr float UPDATE_INTERVAL = 0.05f; // Time in seconds between updates

// --- Grid Representation ---
std::vector<std::vector<bool>> grid(GRID_HEIGHT, std::vector<bool>(GRID_WIDTH));
std::vector<std::vector<bool>> nextGrid(GRID_HEIGHT, std::vector<bool>(GRID_WIDTH));

// --- Shaders ---
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(0.9, 0.9, 0.9, 1.0); // A bright, solid color for cells
    }
)";

// --- Function Declarations ---
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void initialize_grid_randomly();
void update_grid_logic();
GLuint create_shader_program();

// --- Main ---
int main() {
    // --- 1. Initialize GLFW ---
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // --- 2. Create Window ---
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Conway's Game of Life", NULL, NULL);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // --- 3. Initialize GLEW ---
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    // --- 4. Build and Compile Shader Program ---
    GLuint shaderProgram = create_shader_program();

    // --- 5. Setup Vertex Data and Buffers ---
    GLuint vbo, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // --- 6. Initialize Game State ---
    initialize_grid_randomly();
    double lastUpdateTime = 0.0;

    // --- 7. Main Render Loop ---
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();

        // --- Event Handling ---
        glfwPollEvents();

        // --- Update Logic (at a fixed interval) ---
        if (currentTime - lastUpdateTime >= UPDATE_INTERVAL) {
            update_grid_logic();
            lastUpdateTime = currentTime;
        }

        // --- Rendering ---
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // --- Generate and buffer mesh for alive cells ---
        std::vector<float> vertices;
        vertices.reserve(GRID_WIDTH * GRID_HEIGHT * 12); // Pre-allocate memory

        for (size_t y = 0; y < GRID_HEIGHT; ++y) {
            for (size_t x = 0; x < GRID_WIDTH; ++x) {
                if (grid[y][x]) {
                    // Calculate normalized coordinates for the quad
                    float normX = (static_cast<float>(x) / GRID_WIDTH) * 2.0f - 1.0f;
                    float normY = (static_cast<float>(y) / GRID_HEIGHT) * 2.0f - 1.0f;
                    float cellWidth = 2.0f / GRID_WIDTH;
                    float cellHeight = 2.0f / GRID_HEIGHT;

                    // Triangle 1
                    vertices.push_back(normX);              vertices.push_back(normY);
                    vertices.push_back(normX + cellWidth);  vertices.push_back(normY);
                    vertices.push_back(normX);              vertices.push_back(normY + cellHeight);
                    // Triangle 2
                    vertices.push_back(normX + cellWidth);  vertices.push_back(normY);
                    vertices.push_back(normX + cellWidth);  vertices.push_back(normY + cellHeight);
                    vertices.push_back(normX);              vertices.push_back(normY + cellHeight);
                }
            }
        }

        // --- Buffer data and draw ---
        if (!vertices.empty()) {
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 2);
        }

        // --- Swap Buffers ---
        glfwSwapBuffers(window);
    }

    // --- 8. Cleanup ---
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

// --- Function Implementations ---

// Sets the initial state of the grid randomly.
void initialize_grid_randomly() {
    // Use a modern C++ random number generator for better distribution
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 1);

    // Use indexed loops to avoid the std::vector<bool> proxy object issue
    for (size_t y = 0; y < GRID_HEIGHT; ++y) {
        for (size_t x = 0; x < GRID_WIDTH; ++x) {
            grid[y][x] = dist(rng);
        }
    }
}

// Updates the grid based on Conway's Game of Life rules.
void update_grid_logic() {
    for (int y = 0; y < GRID_HEIGHT; ++y) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            int aliveNeighbors = 0;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;

                    // Use modulo for wrap-around (toroidal) grid
                    int nx = (x + dx + GRID_WIDTH) % GRID_WIDTH;
                    int ny = (y + dy + GRID_HEIGHT) % GRID_HEIGHT;

                    if (grid[ny][nx]) {
                        ++aliveNeighbors;
                    }
                }
            }

            // Apply Game of Life rules
            if (grid[y][x]) { // If cell is alive
                nextGrid[y][x] = (aliveNeighbors == 2 || aliveNeighbors == 3);
            }
            else { // If cell is dead
                nextGrid[y][x] = (aliveNeighbors == 3);
            }
        }
    }
    grid = nextGrid; // Swap grids
}

// Callback for window resize events to adjust the viewport.
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Creates and links the vertex and fragment shaders.
GLuint create_shader_program() {
    // --- Vertex Shader ---
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // (Error checking omitted for brevity but recommended in production)

    // --- Fragment Shader ---
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // (Error checking omitted for brevity)

    // --- Shader Program ---
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // (Error checking omitted for brevity)

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}