// shaders/draw.frag
// Fragment shader for drawing. It draws the game texture or the letterbox/pillarbox bars.

#version 330 core
out vec4 FragColor;
in vec2 v_texCoord;

uniform sampler2D u_screenTexture;

void main() {
    // If the panned/zoomed coordinate is outside the [0,1] texture range,
    // draw the background color. This prevents wrapping artifacts.
    if (v_texCoord.x < 0.0 || v_texCoord.x > 1.0 || v_texCoord.y < 0.0 || v_texCoord.y > 1.0) {
        FragColor = vec4(0.05, 0.05, 0.05, 1.0); // Background color
    } else {
        // Otherwise, draw the cell from the simulation texture
        float cellState = texture(u_screenTexture, v_texCoord).r;
        FragColor = vec4(vec3(cellState), 1.0);
    }
}