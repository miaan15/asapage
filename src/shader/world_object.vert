#version 460 core
#extension GL_ARB_shader_draw_parameters : require
#extension GL_ARB_multi_draw_indirect : require

struct MeshVertex {
    vec3 pos;
    float _pad0;
    vec3 normal;
    float _pad1;
    vec2 uv;
    vec2 _pad2;
};

struct InstanceData {
    int mesh_vertex_offset;
    int mesh_index_offset;
    uvec2 _pad0;
    mat4 model_matrix;
};

layout(std140, binding = 0) uniform FrameConstant {
    mat4 view_matrix;
    mat4 projection_matrix;
    float time;
} frame_constant;

layout(std430, binding = 1) readonly buffer InstanceBuf {
    InstanceData instance_buf[];
};

layout(std430, binding = 2) readonly buffer MeshVertexBuf {
    MeshVertex mesh_vertex_buf[];
};

layout(std430, binding = 3) readonly buffer MeshIndexBuf {
    int mesh_index_buf[];
};

layout(location = 0) flat out int o_instance_idx;
layout(location = 1) out vec3 o_world_pos;
layout(location = 2) flat out vec3 o_normal;
layout(location = 3) out vec2 o_uv;

void main() {
    InstanceData instance = instance_buf[gl_DrawID];

    int mesh_index = mesh_index_buf[instance.mesh_index_offset + gl_VertexID];
    MeshVertex vertex = mesh_vertex_buf[instance.mesh_vertex_offset + mesh_index];

    float4 world_pos = instance.model_matrix * vec4(vertex.pos, 1);
    float4 view_pos = frame_constant.view_matrix * world_pos;
    float4 clip_pos = frame_constant.projection_matrix * view_pos;

    gl_Position = clip_pos;

    o_instance_idx = gl_DrawID;
    o_world_pos = world_pos;
    o_normal = mat3(instance.model_matrix) * vertex.normal; // only work with orthogonal transform, which is
    o_uv = vertex.uv;
}
