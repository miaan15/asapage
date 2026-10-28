#version 460 core
#extension GL_ARB_bindless_texture : require

struct MaterialData {
    sampler2D albedo_map;
    vec4 base_color;
};

layout(std430, binding = 4) readonly buffer MaterialBuf {
    MaterialData material_buf[];
};

layout(location = 0) flat in int i_instance_idx;
layout(location = 1) in vec3 i_world_pos;
layout(location = 2) flat in vec3 i_normal;
layout(location = 3) in vec2 i_uv;

layout(location = 0) out vec4 FragColor;

void main() {
    MaterialData mat = material_buf[i_instance_idx];

    vec4 tex_color = texture(mat.albedo_map, i_uv);

    FragColor = tex_color * mat.base_color;
}
