// shaders/draw.frag
// Fragment shader for drawing. It draws the game texture or the letterbox/pillarbox bars.

#version 330 core
out vec4 FragColor;
in vec2 v_texCoord;
in vec2 v_unscaledUv;

uniform sampler2D u_screenTexture;
uniform vec2 u_resolution;
uniform vec2 u_gridResolution;
uniform float u_zoom; // The new uniform

void main() {
    // --- This block must exactly match the logic in the vertex shader ---
    float windowAspect = u_resolution.x / u_resolution.y;
    float gridAspect = u_gridResolution.x / u_gridResolution.y;
    
    vec2 fitScale = vec2(1.0);
    if(windowAspect > gridAspect) {
        fitScale.x = gridAspect / windowAspect;
    } else {
        fitScale.y = windowAspect / gridAspect;
    }
    vec2 fillScale = vec2(1.0);
    float transitionZoom = 1.0 / min(fitScale.x, fitScale.y);
    float t = smoothstep(1.0, transitionZoom, u_zoom);
    vec2 finalScale = mix(fitScale, fillScale, t);
    // --- End of matching block ---


    // Check if the pixel is inside the dynamically scaled area
    bool isInBars = abs(v_unscaledUv.x - 0.5) > finalScale.x * 0.5 || abs(v_unscaledUv.y - 0.5) > finalScale.y * 0.5;
    bool isOutOfBounds = v_texCoord.x < 0.0 || v_texCoord.x > 1.0 || v_texCoord.y < 0.0 || v_texCoord.y > 1.0;

    if (isInBars || isOutOfBounds) {
        FragColor = vec4(0.05, 0.05, 0.05, 1.0); // Dark grey for bars
    } else {
        float cellState = texture(u_screenTexture, v_texCoord).r;
        FragColor = vec4(vec3(cellState), 1.0);
    }
}