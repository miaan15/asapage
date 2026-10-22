#version 450

layout(set = 2, binding = 0) uniform sampler2DArray texture_sampler;

layout(location = 0) in vec4 in_color;
layout(location = 1) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

void main() {
    vec4 texture_color = texture(texture_sampler, vec3(in_uv, 1));

    out_color = texture_color * in_color;
}
