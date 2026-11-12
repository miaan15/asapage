layout(location = 0) flat in int i_object_idx;
layout(location = 1) in vec3 i_world_pos;
layout(location = 2) flat in vec3 i_normal;
layout(location = 3) in vec2 i_uv;

layout(location = 0) out vec4 FragColor;

void main() {
    FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
