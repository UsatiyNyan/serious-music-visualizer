#version 460 core

out vec4 frag_color;

uniform uint u_mode;
uniform vec2 u_window_size;

layout (std430, binding = 0) readonly buffer normalized_freq_domain_output {
    float data[1024];
} nfdo;

float logspace(uint index, uint N) {
    const float logN = log(float(N));
    return exp(float(index) * logN / float(N - 1));
}

float color_coef(uint index, uint N) {
    return index < N ? nfdo.data[index] : 0.0f;
}

vec4 draw_radius(vec2 uv, float radius, bool is_logspace) {
    const uint N = 1024;
    const uint index = uint(length(uv) / radius * N);
    const float color_coef = color_coef(is_logspace ? uint(logspace(index, N)) : index, N);
    const vec3 color_a = vec3(0.0f, 0.0f, 0.0f);
    const vec3 color_b = vec3(0.5f, 0.0f, 0.5f);
    const float alpha = 1.0f;
    return vec4(mix(color_a, color_b, color_coef), alpha);
}

vec4 default_fill() {
    return vec4(1.0f, 0.5f, 0.2f, 1.0f);
}

void main() {
    const vec2 normalized_coord = gl_FragCoord.xy / u_window_size;
    const vec2 aspect_ratio = (u_window_size.x < u_window_size.y)
    ? vec2(1.0f, u_window_size.y / u_window_size.x)
    : vec2(u_window_size.x / u_window_size.y, 1.0f);
    const vec2 uv = (normalized_coord * 2.0f - 1.0f) * aspect_ratio;
    switch (u_mode) {
        case 0: // RADIUS_LINEAR
            frag_color = draw_radius(uv, 1.5f, /*is_logspace=*/false);
            break;
        case 1: // RADIUS_LOG
            frag_color = draw_radius(uv, 1.5f, /*is_logspace=*/true);
            break;
        default:
            frag_color = default_fill();
            break;
    }
}
