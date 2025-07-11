// shaders/draw.frag
// Fragment shader for drawing. It draws the game texture or the letterbox/pillarbox bars.

#version 330 core
out vec4 FragColor;
in vec2 v_texCoord;
in vec2 v_unscaledUv; // Use this for bounds checking

uniform sampler2D u_screenTexture;
uniform vec2 u_resolution;
uniform vec2 u_gridResolution;

void main() {
    float windowAspect = u_resolution.x / u_resolution.y;
    float gridAspect = u_gridResolution.x / u_gridResolution.y;
    
    vec2 scale;
    if(windowAspect > gridAspect) {
        scale = vec2(gridAspect / windowAspect, 1.0);
    } else {
        scale = vec2(1.0, windowAspect / gridAspect);
    }

    // Check if the original screen UV is outside the scaled area
    if (abs(v_unscaledUv.x - 0.5) > scale.x * 0.5 || abs(v_unscaledUv.y - 0.5) > scale.y * 0.5) {
        FragColor = vec4(0.05, 0.05, 0.05, 1.0); // Dark grey for bars
    } else {
        // Use the corrected, panned, and zoomed texture coordinates
        // We also check if the coordinate is outside the [0,1] range to avoid wrapping artifacts at the edges when zoomed in.
        if (v_texCoord.x < 0.0 || v_texCoord.x > 1.0 || v_texCoord.y < 0.0 || v_texCoord.y > 1.0) {
             FragColor = vec4(0.05, 0.05, 0.05, 1.0); // Same color as letterbox
        } else {
             float cellState = texture(u_screenTexture, v_texCoord).r;
             FragColor = vec4(vec3(cellState), 1.0);
        }
    }
}