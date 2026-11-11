layout(location = 0) in int i_object_idx;

layout(location = 0) flat out int o_object_idx;

layout(location = 1) out vec3 o_world_pos;
layout(location = 2) flat out vec3 o_normal;
layout(location = 3) out vec2 o_uv;

void main() {
    Object object = object_list[i_object_idx];

    int mesh_index = mesh_index_buf[object.mesh_index_offset + gl_VertexID];
    MeshVertex vertex = mesh_vertex_buf[object.mesh_vertex_offset + mesh_index];

    vec4 world_pos = object.model_matrix * vec4(vertex.pos, 1);
    vec4 view_pos = frame_constant.view_matrix * world_pos;
    vec4 clip_pos = frame_constant.projection_matrix * view_pos;

    gl_Position = clip_pos;

    o_object_idx = i_object_idx;
    o_world_pos = world_pos.xyz;
    o_normal = mat3(object.model_matrix) * vertex.normal; // only work with orthogonal transform, which it is here
    o_uv = vertex.uv;
}
