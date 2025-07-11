// shaders/draw.vert
// Vertex shader for drawing. It calculates aspect-ratio-correct texture coordinates.

#version 330 core
layout (location = 0) in vec2 aPos;

out vec2 v_texCoord;
out vec2 v_unscaledUv;

uniform vec2 u_pan;
uniform float u_zoom;
uniform vec2 u_resolution;
uniform vec2 u_gridResolution;

void main() {
    v_unscaledUv = (aPos + 1.0) / 2.0;
    vec2 uv = v_unscaledUv;

    // --- Calculate the two modes: FIT and FILL ---
    float windowAspect = u_resolution.x / u_resolution.y;
    float gridAspect = u_gridResolution.x / u_gridResolution.y;
    
    // 1. FIT scale (with bars) - your original calculation
    vec2 fitScale = vec2(1.0);
    if (windowAspect > gridAspect) {
        fitScale.x = gridAspect / windowAspect;
    } else {
        fitScale.y = windowAspect / gridAspect;
    }

    // 2. FILL scale (no bars)
    vec2 fillScale = vec2(1.0);

    // --- Smoothly transition between the two modes ---
    // The zoom level where the view is completely filled
    float transitionZoom = 1.0 / min(fitScale.x, fitScale.y);

    // An interpolation factor from 0.0 to 1.0
    // smoothstep creates a nice ease-in/out effect
    float t = smoothstep(1.0, transitionZoom, u_zoom);

    // Linearly interpolate between the two scale modes
    vec2 finalScale = mix(fitScale, fillScale, t);
    
    // --- Apply the final transformations ---
    uv = (uv - 0.5) * finalScale + 0.5;
    
    v_texCoord = uv / u_zoom + u_pan;
    
    gl_Position = vec4(aPos, 0.0, 1.0);
}