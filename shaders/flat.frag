#version 460 core

out vec4 frag_color;

uniform uint u_mode;
uniform float u_time = 0;
uniform vec2 u_window_size;

layout(std430, binding = 0) readonly buffer normalized_freq_domain_output {
    float data[1024];
} nfdo;

vec4 default_fill() {
    return vec4(1.0f, 0.5f, 0.2f, 1.0f);
}

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

const uint rm_TYPE_NONE = 0;
const uint rm_TYPE_REFLECT = 1;
const uint rm_TYPE_SOLID = 2;
const uint rm_TYPE_SHADE = 3;
const uint rm_TYPE_NORMAL = 4;
const uint rm_TYPE_DISTANCE = 5;
struct rm_MO {
    vec3 c; // color
    float d; // distance
    uint t; // type
};

const float rm_MIN_DISTANCE = 0.001;
const float rm_MAX_DISTANCE = 1000.0;
const float rm_SMOOTHING = 0.1;

float rm_smooth_min(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

void rm_smooth_union(inout rm_MO mo, float d, vec3 c, uint t) {
    const float k = rm_SMOOTHING;
    const bool wins = d < mo.d;
    mo.d = rm_smooth_min(mo.d, d, k);
    if (wins) {
        mo.c = c;
        mo.t = t;
    }
}

vec3 rm_twist(vec3 p, float k) {
    const float c = cos(k * p.y);
    const float s = sin(k * p.y);
    const mat2 m = mat2(c, -s, s, c);
    const vec3 q = vec3(m * p.xz, p.y);
    return q;
}

float rm_sd_sphere(vec3 p, vec3 c, float r) {
    return length(p - c) - r;
}

float rm_sd_torus(vec3 p, vec2 t)
{
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

rm_MO rm_map_the_world(vec3 p) {
    rm_MO mo;
    mo.d = rm_MAX_DISTANCE;

    // {
    //     const float sphere = rm_sd_sphere(p, vec3(1.0, 0.0, 0.0), 1.0);
    //     rm_smooth_union(mo, sphere, vec3(1.0, 0.0, 0.0), rm_TYPE_SOLID);
    // }
    //
    // {
    //     const float sphere = rm_sd_sphere(p, vec3(-1.0, 0.0, 0.0), 1.0);
    //     rm_smooth_union(mo, sphere, vec3(0.0, 1.0, 0.0), rm_TYPE_SOLID);
    // }

    {
        const float sphere = rm_sd_sphere(p, vec3(0.0, 0.0, 0.0), 1.0);
        rm_smooth_union(mo, sphere, vec3(0.0), rm_TYPE_REFLECT);
    }

    {
        const float torus = rm_sd_torus(rm_twist(p, 1.0), vec2(4.0, 1.0));
        rm_smooth_union(mo, torus, vec3(0.7, 0.1, 0.25), rm_TYPE_SOLID);
    }

    return mo;
}

vec3 rm_calculate_normal(in vec3 p)
{
    const float eps = 0.001;
    const vec3 small_step = vec3(eps, 0.0, 0.0);

    const float gradient_x = rm_map_the_world(p + small_step.xyy).d - rm_map_the_world(p - small_step.xyy).d;
    const float gradient_y = rm_map_the_world(p + small_step.yxy).d - rm_map_the_world(p - small_step.yxy).d;
    const float gradient_z = rm_map_the_world(p + small_step.yyx).d - rm_map_the_world(p - small_step.yyx).d;

    return normalize(vec3(gradient_x, gradient_y, gradient_z));
}

mat2 rm_rot_mat_2d(float angle) {
    const float s = sin(angle);
    const float c = cos(angle);
    return mat2(
        c, -s,
        s, c
    );
}

vec3 rm_rot_xz_by(vec3 p, mat2 r) {
    p.xz *= r;
    return p;
}

vec3 rm_ray_march(vec3 ray_origin, vec3 ray_direction) {
    const uint MAX_STEPS = 32;
    const uint MAX_BOUNCES = 8;
    const float ANGULAR_SPEED = 0.3;

    const mat2 rotation = rm_rot_mat_2d(u_time * ANGULAR_SPEED);
    const mat2 inverse_rotation = transpose(rotation);
    ray_origin = rm_rot_xz_by(ray_origin, inverse_rotation);
    ray_direction = rm_rot_xz_by(ray_direction, inverse_rotation);

    const vec3 light_position = vec3(0.0, 10.0, 10.0);

    float distance_traveled = 0.0;
    for (uint step_count = 0; step_count < MAX_STEPS; ++step_count) { // render loop
        const vec3 current_position = ray_origin + (ray_direction * distance_traveled);
        const rm_MO closest_obj = rm_map_the_world(current_position);

        if (closest_obj.d < rm_MIN_DISTANCE) {
            switch (closest_obj.t) {
                case rm_TYPE_NONE:
                break;
                case rm_TYPE_REFLECT:
                {
                    const vec3 normal = rm_calculate_normal(current_position);
                    ray_direction = reflect(ray_direction, normal);
                    ray_origin = current_position + normal * rm_MIN_DISTANCE * 2.0;
                    distance_traveled = 0.1;
                    break;
                }
                case rm_TYPE_SOLID:
                {
                    return closest_obj.c;
                }
                case rm_TYPE_SHADE:
                {
                    const vec3 direction_to_light = normalize(current_position - light_position);
                    const vec3 normal = rm_calculate_normal(current_position);
                    const float diffuse_intensity = max(0.0, dot(normal, direction_to_light));
                    return closest_obj.c * diffuse_intensity;
                }
                case rm_TYPE_NORMAL:
                {
                    return abs(rm_calculate_normal(current_position));
                }
                case rm_TYPE_DISTANCE:
                {
                    const float x = (1 / distance_traveled);
                    return vec3(x + 0.2, x, 0.2 + x);
                }
                default: // error
                return vec3(1.0, 0.0, 0.0);
            }
        }

        if (distance_traveled > rm_MAX_DISTANCE) {
            break;
        }
        distance_traveled += closest_obj.d;
    }

    return vec3(0.0);
}

vec4 draw_ray_marching(vec2 uv) {
    const vec3 ray_origin = vec3(0.0, 0.0, -10.0);
    const vec3 ray_direction = vec3(uv, 1.0);

    const vec3 shaded_color = rm_ray_march(ray_origin, ray_direction);

    return vec4(shaded_color, 1.0);
}

void main() {
    const vec2 normalized_coord = gl_FragCoord.xy / u_window_size;
    const vec2 aspect_ratio = (u_window_size.x < u_window_size.y) //
        ? vec2(1.0f, u_window_size.y / u_window_size.x) //
        : vec2(u_window_size.x / u_window_size.y, 1.0f);
    const vec2 uv = (normalized_coord * 2.0f - 1.0f) * aspect_ratio;
    switch (u_mode) {
        case 1: // RADIUS_LINEAR
        frag_color = draw_radius(uv, 1.5f, /*is_logspace=*/ false);
        break;
        case 2: // RADIUS_LOG
        frag_color = draw_radius(uv, 1.5f, /*is_logspace=*/ true);
        break;
        case 3: // RAY_MARCHING
        frag_color = draw_ray_marching(uv);
        break;
        default:
        frag_color = default_fill();
        break;
    }
}
