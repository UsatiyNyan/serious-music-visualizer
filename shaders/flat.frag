#version 330 core

out vec4 frag_color;

uniform uint u_mode;
uniform float u_time = 0;
uniform vec2 u_window_size;
uniform vec3 u_ray_origin;
uniform float u_ray_pitch;
uniform float u_sound_level = 0; // [0, 1]

#define M_PI 3.1415926535897932384626433832795

const uint nfdo_N = 1024u;
const float nfdo_logN = log(float(nfdo_N));

uniform samplerBuffer nfdo;

vec4 default_fill() {
    return vec4(1.0f, 0.5f, 0.2f, 1.0f);
}

float logspace(float v) {
    return exp(v * nfdo_logN) / float(nfdo_N);
}

float nfdo_at(uint index) {
    if (index < nfdo_N) {
        return clamp(texelFetch(nfdo, int(index)).r, -1.0, 1.0);
    } else {
        return 0.0;
    }
}

float nfdo_at_smoothed(float v) {
    float u = v * float(nfdo_N);
    uint i = uint(int(floor(u)));
    return mix(nfdo_at(i), nfdo_at(i + 1u), fract(u));
}

vec4 draw_radius(vec2 uv, float radius, bool is_logspace) {
    float v = length(uv) / radius;
    float color_coef = nfdo_at_smoothed(is_logspace ? logspace(v) : v);
    const vec3 color_a = vec3(0.0f, 0.0f, 0.0f);
    const vec3 color_b = vec3(0.5f, 0.0f, 0.5f);
    const float alpha = 1.0f;
    return vec4(mix(color_a, color_b, color_coef), alpha);
}

const uint rm_TYPE_NONE = 0u;
const uint rm_TYPE_REFLECT = 1u;
const uint rm_TYPE_SOLID = 2u;
const uint rm_TYPE_SHADE = 3u;
const uint rm_TYPE_NORMAL = 4u;
const uint rm_TYPE_DISTANCE = 5u;
struct rm_MO {
    vec3 c; // color
    float d; // distance
    uint t; // type
};

const float rm_MIN_DISTANCE = 0.001;
const float rm_MAX_DISTANCE = 10000.0;
const float rm_SMOOTHING = 0.1;

float rm_smooth_min(float a, float b, float k) {
    float h = clamp(0.5 + 0.5 * (b - a) / k, 0.0, 1.0);
    return mix(b, a, h) - k * h * (1.0 - h);
}

void rm_smooth_union(inout rm_MO mo, float d, vec3 c, uint t) {
    const float k = rm_SMOOTHING;
    bool wins = d < mo.d;
    mo.d = rm_smooth_min(mo.d, d, k);
    if (wins) {
        mo.c = c;
        mo.t = t;
    }
}

vec3 rm_twist(vec3 p, float k) {
    float c = cos(k * p.y);
    float s = sin(k * p.y);
    mat2 m = mat2(c, -s, s, c);
    vec3 q = vec3(m * p.xz, p.y);
    return q;
}

float rm_sd_sphere(vec3 p, vec3 c, float r) {
    return length(p - c) - r;
}

float rm_sd_torus(vec3 p, vec3 c, float R, float r) {
    vec3 d = p - c;
    vec2 q = vec2(length(d.xz) - R, d.y);
    return length(q) - r;
}

float rm_sd_disk(vec3 p, float radius) {
    return max(length(p.xz) - radius, abs(p.y));
}

rm_MO rm_map_the_world(vec3 p) {
    rm_MO mo;
    mo.d = rm_MAX_DISTANCE;
    mo.t = rm_TYPE_NONE;

    {
        vec3 c = vec3(0.0, 2.0 + sin(u_time * 0.3), 0.0);
        float sphere = rm_sd_sphere(p, c, 0.5);
        rm_smooth_union(mo, sphere, vec3(0.0), rm_TYPE_REFLECT);
    }

    {
        float torus = rm_sd_torus(rm_twist(p, u_sound_level * 4), vec3(0.0), 8.0, 1.0);
        rm_smooth_union(mo, torus, vec3(0.7, 0.1, 0.25), rm_TYPE_SOLID);
    }

    {
        const float R = 4.0;
        const float r = 0.1;
        const float ypos = -0.5;
        float pr = length(p.xz);
        if (pr <= R) {
            float v = clamp(pr / R, 0.0, 1.0);
            float logspace_v = logspace(v);
            float nfdo_value = nfdo_at_smoothed(logspace_v); // [-1, 1]
            float h = (nfdo_value + 1) / 2; // [0, 1]
            float puddle_torus = rm_sd_torus(p, vec3(0.0, h - ypos, 0.0), pr, r);
            vec3 color = mix(vec3(1.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0), logspace_v);
            rm_smooth_union(mo, puddle_torus, mix(vec3(0.0), color, h), rm_TYPE_SOLID);
        }
    }

    return mo;
}

vec3 rm_calculate_normal(in vec3 p)
{
    const float eps = 0.001;
    const vec3 small_step = vec3(eps, 0.0, 0.0);

    float gradient_x = rm_map_the_world(p + small_step.xyy).d - rm_map_the_world(p - small_step.xyy).d;
    float gradient_y = rm_map_the_world(p + small_step.yxy).d - rm_map_the_world(p - small_step.yxy).d;
    float gradient_z = rm_map_the_world(p + small_step.yyx).d - rm_map_the_world(p - small_step.yyx).d;

    return normalize(vec3(gradient_x, gradient_y, gradient_z));
}

mat2 rm_rot_mat_2d(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat2(
        c, -s,
        s, c
    );
}

vec3 rm_rot_xz_by(vec3 p, mat2 r) {
    p.xz *= r;
    return p;
}

mat3 rm_rot_x(float a) {
    float s = sin(a);
    float c = cos(a);
    return mat3(
        1.0, 0.0, 0.0,
        0.0, c, -s,
        0.0, s, c
    );
}

mat3 rm_rot_y(float a) {
    float s = sin(a);
    float c = cos(a);
    return mat3(
        c, 0.0, s,
        0.0, 1.0, 0.0,
        -s, 0.0, c
    );
}

vec3 rm_ray_march(vec3 ray_origin, vec3 ray_direction) {
    const uint MAX_STEPS = 32u;
    const uint MAX_BOUNCES = 8u;
    const float ANGULAR_SPEED = 0.3;

    mat2 rotation = rm_rot_mat_2d(u_time * ANGULAR_SPEED);
    // const mat2 rotation = rm_rot_mat_2d(0.0);
    mat2 inverse_rotation = transpose(rotation);
    ray_origin = rm_rot_xz_by(ray_origin, inverse_rotation);
    ray_direction = rm_rot_xz_by(ray_direction, inverse_rotation);

    const vec3 light_position = vec3(0.0, 5.0, 0.0);

    float distance_traveled = 0.0;
    for (uint step_count = 0u; step_count < MAX_STEPS; ++step_count) { // render loop
        vec3 current_position = ray_origin + (ray_direction * distance_traveled);
        rm_MO closest_obj = rm_map_the_world(current_position);

        if (closest_obj.d < rm_MIN_DISTANCE) {
            switch (closest_obj.t) {
                case rm_TYPE_NONE:
                break;
                case rm_TYPE_REFLECT:
                {
                    vec3 normal = rm_calculate_normal(current_position);
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
                    vec3 direction_to_light = normalize(current_position - light_position);
                    vec3 normal = rm_calculate_normal(current_position);
                    float diffuse_intensity = max(0.0, dot(normal, direction_to_light));
                    return closest_obj.c * diffuse_intensity;
                }
                case rm_TYPE_NORMAL:
                {
                    return abs(rm_calculate_normal(current_position));
                }
                case rm_TYPE_DISTANCE:
                {
                    float x = (1 / distance_traveled);
                    return vec3(x + 0.2, x, 0.2 + x);
                }
                default: // error
                return vec3(1.0, 0.0, 0.0);
            }
        }

        if (distance_traveled > rm_MAX_DISTANCE) {
            break;
        }
        distance_traveled += min(closest_obj.d, 0.5);
        ;
    }

    return vec3(0.0);
}

vec4 draw_ray_marching(vec2 uv) {
    vec3 ray_origin = u_ray_origin;
    vec3 ray_direction = normalize(rm_rot_x(-(M_PI * u_ray_pitch)) * vec3(uv, 1.0));

    vec3 shaded_color = rm_ray_march(ray_origin, ray_direction);

    return vec4(shaded_color, 1.0);
}

void main() {
    vec2 normalized_coord = gl_FragCoord.xy / u_window_size;
    vec2 aspect_ratio = (u_window_size.x < u_window_size.y) //
        ? vec2(1.0f, u_window_size.y / u_window_size.x) //
        : vec2(u_window_size.x / u_window_size.y, 1.0f);
    vec2 uv = (normalized_coord * 2.0f - 1.0f) * aspect_ratio;
    switch (u_mode) {
        case 1u: // RADIUS_LINEAR
        frag_color = draw_radius(uv, 1.5f, /*is_logspace=*/ false);
        break;
        case 2u: // RADIUS_LOG
        frag_color = draw_radius(uv, 1.5f, /*is_logspace=*/ true);
        break;
        case 3u: // RAY_MARCHING
        frag_color = draw_ray_marching(uv);
        break;
        default:
        frag_color = default_fill();
        break;
    }
}
