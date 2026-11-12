layout(location = 0) in int i_object_idx;

layout(location = 0) flat out int o_object_idx;

layout(location = 1) out vec3 o_world_pos;
layout(location = 2) flat out vec3 o_normal;
layout(location = 3) out vec2 o_uv;

void main() {
    const vec2 pos[3] = vec2[3](
        vec2(-0.5, -0.5),
        vec2( 0.5, -0.5),
        vec2( 0.0,  0.5)
    );

    int id = gl_VertexID % 3;
    gl_Position = vec4(pos[id], 0.0, 1.0);

    o_object_idx = 0;
    o_world_pos  = vec3(pos[id], 0.0);
    o_normal     = vec3(0.0, 0.0, 1.0);
    o_uv         = pos[id] * 0.5 + 0.5;}
