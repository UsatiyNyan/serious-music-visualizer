#version 460 core

out vec4 frag_color;

uniform uint u_mode;

vec4 draw_circles() {
    return vec4(0.0);
}

vec4 default_fill() {
    return vec4(1.0f, 0.5f, 0.2f, 1.0f);
}

void main() {
    switch (u_mode) {
        case 0:
            frag_color = draw_circles();
            break;
        default:
            frag_color = default_fill();
            break;
    }
}
