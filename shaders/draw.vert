// shaders/draw.vert
// Vertex shader for drawing. It calculates aspect-ratio-correct texture coordinates.
#version 330 core
layout (location = 0) in vec2 aPos;

out vec2 v_texCoord;

uniform vec2 u_pan;
uniform float u_zoom;
uniform vec2 u_resolution;
uniform vec2 u_gridResolution;

void main() {
    // 1. First, calculate the correct texture coordinates for sampling.
    // This applies the pan and zoom to the raw quad coordinates.
    // The result is what part of the texture we want to see.
    v_texCoord = ((aPos + 1.0) / 2.0) / u_zoom + u_pan;

    // 2. Separately, calculate the aspect-ratio-correct scale for drawing.
    // This ensures the grid isn't stretched, creating letterboxes if needed.
    float windowAspect = u_resolution.x / u_resolution.y;
    float gridAspect = u_gridResolution.x / u_gridResolution.y;
    vec2 scale = vec2(1.0);
    if (windowAspect > gridAspect) {
        scale.x = gridAspect / windowAspect;
    } else {
        scale.y = windowAspect / gridAspect;
    }

    // 3. Apply the aspect ratio scale to the vertex position.
    // This scales the entire quad down to fit the screen correctly.
    gl_Position = vec4(aPos * scale, 0.0, 1.0);
}