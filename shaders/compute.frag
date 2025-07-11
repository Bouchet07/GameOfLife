// shaders/compute.frag
// The core Game of Life logic, executed entirely on the GPU.

#version 330 core
out vec4 FragColor;
in vec2 v_texCoord;

uniform sampler2D u_currentState;

// Function to get the state of a cell, handling wrapping automatically due to GL_REPEAT
float getCellState(vec2 coord) {
    return texture(u_currentState, coord).r;
}

void main() {
    // Calculate the size of a single pixel in texture coordinates
    vec2 pixel = 1.0 / textureSize(u_currentState, 0);
    
    int aliveNeighbors = 0;
    
    // Sum the states of the 8 neighbors
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            // texture() with GL_REPEAT handles wrapping around the edges
            aliveNeighbors += int(getCellState(v_texCoord + vec2(float(dx), float(dy)) * pixel));
        }
    }
    
    float currentState = getCellState(v_texCoord);
    aliveNeighbors -= int(currentState); // Exclude the current cell from the count
    float newState = 0.0;
    
    // Apply Conway's Game of Life rules
    // Rule 1: A living cell with 2 or 3 living neighbours lives on.
    if (currentState > 0.5 && (aliveNeighbors == 2 || aliveNeighbors == 3)) {
        newState = 1.0;
    } 
    // Rule 2: A dead cell with exactly 3 living neighbours becomes a living cell.
    else if (currentState < 0.5 && aliveNeighbors == 3) {
        newState = 1.0;
    }
    // All other cells die or stay dead.
    
    FragColor = vec4(vec3(newState), 1.0);
}