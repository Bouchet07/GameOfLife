// shaders/draw.vert
// Vertex shader for drawing. It calculates aspect-ratio-correct texture coordinates.

#version 330 core
layout (location = 0) in vec2 aPos;

out vec2 v_texCoord;
out vec2 v_unscaledUv; // Pass this to fragment shader for bounds checking

uniform vec2 u_pan;
uniform float u_zoom;
uniform vec2 u_resolution;
uniform vec2 u_gridResolution;

void main() {
    // This is the [0,1] coordinate of the screen pixel
    v_unscaledUv = (aPos + 1.0) / 2.0; 
    vec2 uv = v_unscaledUv;

    float windowAspect = u_resolution.x / u_resolution.y;
    float gridAspect = u_gridResolution.x / u_gridResolution.y;
    
    vec2 scale;
    if (windowAspect > gridAspect) {
        // Window is wider than grid (pillarbox)
        scale = vec2(gridAspect / windowAspect, 1.0);
    } else {
        // Window is taller than grid (letterbox)
        scale = vec2(1.0, windowAspect / gridAspect);
    }
    
    // Scale the coordinate space, then center it
    uv = (uv - 0.5) * scale + 0.5;
    
    // Apply pan and zoom to the corrected coordinates
    v_texCoord = uv / u_zoom + u_pan;
    gl_Position = vec4(aPos, 0.0, 1.0);
}