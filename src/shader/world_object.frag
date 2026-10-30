layout(location = 0) flat in int i_instance_idx;
layout(location = 1) in vec3 i_world_pos;
layout(location = 2) flat in vec3 i_normal;
layout(location = 3) in vec2 i_uv;

layout(location = 0) out vec4 FragColor;

void main() {
    InstanceData instance = instance_buf[i_instance_idx];

    Material material = material_buf[instance.material_offset];

    // FIXME
    vec3 light_pos = vec3(5, 10, 5);

    vec3 light_dir = normalize(light_pos - i_world_pos);
    vec3 reflected_dir = reflect(-light_dir, i_normal);
    vec3 view_dir = normalize(frame_constant.eye_pos - i_world_pos);

    vec3 ambient_color = material.base_color.xyz * 0.1;
    vec3 diffuse_color = max(0, dot(light_dir, i_normal)) * material.base_color.xyz * 0.5;
    vec3 specular_color = pow(max(0, dot(reflected_dir, view_dir)), 1) * vec3(1) * 0.5;

    FragColor = vec4(ambient_color + (diffuse_color + specular_color) * 1, 1);
}
