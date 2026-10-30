layout(location = 0) in int i_instance_idx;

layout(location = 0) flat out int o_instance_idx;
layout(location = 1) out vec3 o_world_pos;
layout(location = 2) flat out vec3 o_normal;
layout(location = 3) out vec2 o_uv;

void main() {
    InstanceData instance = instance_buf[i_instance_idx];

    int mesh_index = mesh_index_buf[instance.mesh_index_offset + gl_VertexID];
    MeshVertex vertex = mesh_vertex_buf[instance.mesh_vertex_offset + mesh_index];

    vec4 world_pos = instance.model_matrix * vec4(vertex.pos, 1);
    vec4 view_pos = frame_constant.view_matrix * world_pos;
    vec4 clip_pos = frame_constant.projection_matrix * view_pos;

    gl_Position = clip_pos;

    o_instance_idx = i_instance_idx;
    o_world_pos = world_pos.xyz;
    o_normal = mat3(instance.model_matrix) * vertex.normal; // only work with orthogonal transform, which is
    o_uv = vertex.uv;
}
