#version 460 core

layout(std140, binding = 0) uniform FrameConstant {
    mat4 view_matrix;
    mat4 projection_matrix;
} frame_constant;

struct InstanceData {
    int mesh_vertex_offset;
    int mesh_index_offset;
    int material_offset;
    float _pad0;
    mat4 model_matrix;
};
layout(std430, binding = 1) readonly buffer InstanceBuf {
    InstanceData instance_buf[];
};

struct Material {
    vec4 base_color;
};
layout(std430, binding = 4) readonly buffer MaterialBuf {
    Material material_buf[];
};

layout(location = 0) flat in int i_instance_idx;
layout(location = 1) in vec3 i_world_pos;
layout(location = 2) flat in vec3 i_normal;
layout(location = 3) in vec2 i_uv;

layout(location = 0) out vec4 FragColor;

void main() {
    InstanceData instance = instance_buf[i_instance_idx];

    Material material = material_buf[instance.material_offset];

    FragColor = material.base_color;
}
