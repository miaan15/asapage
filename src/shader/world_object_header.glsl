#version 430 core

// frame constant
layout(std140, binding = 0) uniform FrameConstant {
    mat4 view_matrix;
    mat4 projection_matrix;
    vec3 eye_pos;
    float _pad0;
} frame_constant;

// object3d
struct Object {
    int mesh_vertex_offset;
    int mesh_index_offset;

    int material_idx;

    float _pad0;

    mat4 model_matrix;
};
layout(std430, binding = 1) readonly buffer InstanceBuf {
    Object object_list[];
};

// mesh
struct MeshVertex {
    vec3 pos;
    float _pad0;
    vec3 normal;
    float _pad1;
    vec2 uv;
    float _pad2;
    float _pad3;
};
layout(std430, binding = 2) readonly buffer MeshVertexBuf {
    MeshVertex mesh_vertex_buf[];
};

layout(std430, binding = 3) readonly buffer MeshIndexBuf {
    int mesh_index_buf[];
};

// material
struct Material {
    vec3 albedo;
    float _pad0;
};
layout(std430, binding = 4) readonly buffer MaterialBuf {
    Material material_list[];
};
