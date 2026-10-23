#version 450

// per instance
struct PerInstanceAccessor {
    uint model_idx;
    uint transform_idx;
    uint color_idx;
};
layout(set = 2, binding = 0) readonly buffer PerInstanceAccessorBuf {
    PerInstanceAccessor instance_accessors[];
};

// per vertex
struct PerVertexAccessor {
    uint placeholder;
};
layout(set = 2, binding = 1) readonly buffer PerVertexAccessorBuf {
    PerVertexAccessor vertex_accessors[];
};

// color
struct Color {
    vec3 color;
};
layout(set = 2, binding = 2) readonly buffer ColorBuf {
    Color colors[];
};

// env
layout(set = 2, binding = 3) readonly buffer EnvBuf {
    vec3 env_light_pos;
    float env_light_intensity;
};

// view
layout(set = 2, binding = 4) readonly buffer ViewBuf {
    vec2 view_size;
    vec3 view_eye;
    vec3 view_center;
    vec3 view_up;
    vec2 view_range;
    float view_fov;
};

layout(location = 0) flat in uint instance_idx;
layout(location = 1) in vec3 i_pos;
layout(location = 2) flat in vec3 fi_normal;

layout(location = 0) out vec4 o_color;

void main() {
    uint color_idx = instance_accessors[instance_idx].color_idx;
    vec3 albedo = colors[color_idx].color;

    vec3 light_dir = normalize(env_light_pos - i_pos);
    vec3 view_dir = normalize(view_eye - i_pos);
    vec3 reflect_light_dir = reflect(-light_dir, fi_normal);

    float ambient_strength = 0.1; // FIXME
    vec3 ambient = ambient_strength * albedo;

    float diff = max(dot(fi_normal, light_dir), 0);
    vec3 diffuse = diff * albedo * 1;

    float shininess = 10; // FIXME
    float spec = pow(max(dot(reflect_light_dir, view_dir), 0), shininess);
    vec3 specular = vec3(1) * spec * 0.5;

    vec3 final_color = ambient + (diffuse + specular) * env_light_intensity;
    o_color = vec4(final_color, 1.0);
}
