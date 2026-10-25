#version 450

layout(std430, set = 2, binding = 0) readonly buffer EnvBuf {
    vec3 env_light_pos;
    float env_light_intensity;
};

layout(std430, set = 2, binding = 1) readonly buffer ViewBuf {
    vec2 view_size;
    vec3 view_eye;
    vec3 view_center;
    vec3 view_up;
    vec2 view_range;
    float view_fov;
};

layout(location = 0) in vec3 i_pos;
layout(location = 1) flat in vec3 i_normal;
layout(location = 2) in vec2 i_uv;
layout(location = 3) in vec3 i_color;

layout(location = 0) out vec4 o_color;

void main() {
    vec3 normal_dir = i_normal;
    vec3 light_dir = normalize(env_light_pos - i_pos);
    vec3 view_dir = normalize(view_eye - i_pos);
    vec3 reflect_light_dir = reflect(-light_dir, i_normal);

    vec3 ambient = i_color * 0.1;

    vec3 diffuse = max(dot(i_normal, light_dir), 0) * i_color * 0.8;

    vec3 specular = pow(max(dot(reflect_light_dir, view_dir), 0), 2.0) * vec3(1) * 0.5;

    vec3 color = ambient + (diffuse + specular) * env_light_intensity;

    // o_color = texture(texture_sampler, i_uv) * vec4(color, 1.0);
    o_color = vec4(color, 1);
}
