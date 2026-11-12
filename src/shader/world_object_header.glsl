#version 430 core

// frame constant
layout(std140, binding = 0) uniform FrameConstant {
    mat4 view_matrix;
    mat4 projection_matrix;
    vec3 eye_pos;
} frame_constant;

// object3d
struct Object {
    int mesh_vertex_offset;
    int mesh_index_offset;

    int material_idx;

    mat4 model_matrix;
};
layout(std430, binding = 1) readonly buffer ObjectBuf {
    Object object_list[];
};

// mesh
struct MeshVertex {
    vec3 pos;
    vec3 normal;
    vec2 uv;
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
};
layout(std430, binding = 4) readonly buffer MaterialBuf {
    Material material_list[];
};
