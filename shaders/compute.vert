// shaders/compute.vert
// Passthrough vertex shader for the computation step.

#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 v_texCoord;

void main() {
    v_texCoord = (aPos + 1.0) / 2.0;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
