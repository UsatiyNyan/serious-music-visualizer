#version 460 core

layout (location = 0) in vec2 in_pos;

uniform mat4 u_transform = mat4(1.0);

void main() {
    gl_Position = u_transform * vec4(in_pos, 0.0, 1.0);
}
