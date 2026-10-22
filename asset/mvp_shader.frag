#version 450

layout(set = 2, binding = 0) readonly buffer ColorBuf {
    vec3 color;
};

layout(set = 2, binding = 1) readonly buffer ViewBuf {
    vec2 view_size;
    vec3 view_eye;
    vec3 view_center;
    vec3 view_up;
    vec2 view_range;
    float view_fov;
};

layout(set = 2, binding = 2) readonly buffer EnvBuf {
    vec3 env_light_pos;
    float env_light_intensity;
};

layout(location = 0) in vec3 i_pos;
layout(location = 1) flat in vec3 fi_normal;

layout(location = 0) out vec4 o_color;

void main() {
    vec3 light_dir = normalize(i_pos - env_light_pos);
    vec3 reflect_light_dir = (2.0 * dot(fi_normal, -light_dir) * fi_normal) + light_dir;
    vec3 view_dir = normalize(view_eye - i_pos);

    float light_val = pow(max(0.0, dot(reflect_light_dir, view_dir)), env_light_intensity);

    o_color = vec4(color * .7 + light_val * .5, 1);
}
