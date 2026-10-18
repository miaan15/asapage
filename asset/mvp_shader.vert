#version 450

layout(location = 0) in vec3 i_pos;

layout(set = 1, binding = 0) uniform Transform {
    vec3 u_translation;
    vec4 u_rotation;
    vec3 u_scale;
};
layout(set = 1, binding = 1) uniform Color {
    vec3 u_color;
};

layout(location = 0) out vec4 frag_color;

mat4 translation_matrix(vec3 t) {
    return mat4(
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        t.x, t.y, t.z, 1.0
    );
}

mat4 rotation_matrix(vec4 r) {
    r = normalize(r);
    float x = r.x, y = r.y, z = r.z, w = r.w;
    return mat4(
        1.0 - 2.0*(y*y + z*z), 2.0*(x*y + z*w),       2.0*(x*z - y*w),       0.0,
        2.0*(x*y - z*w),       1.0 - 2.0*(x*x + z*z), 2.0*(y*z + x*w),       0.0,
        2.0*(x*z + y*w),       2.0*(y*z - x*w),       1.0 - 2.0*(x*x + y*y), 0.0,
        0.0,                   0.0,                   0.0,                   1.0
    );
}

mat4 scale_matrix(vec3 s) {
    return mat4(
        s.x, 0.0, 0.0, 0.0,
        0.0, s.y, 0.0, 0.0,
        0.0, 0.0, s.z, 0.0,
        0.0, 0.0, 0.0, 1.0
    );
}

void main() {
    mat4 T = translation_matrix(u_translation);
    mat4 R = rotation_matrix(u_rotation);
    mat4 S = scale_matrix(u_scale);

    mat4 model_matrix = T * R * S;

    gl_Position = model_matrix * vec4(i_pos, 1.0);

    // frag_color.x = u_color.x * ((gl_Position.x + 1.0) * 0.5);
    // frag_color.y = u_color.y * ((gl_Position.y + 1.0) * 0.5);
    // frag_color.z = u_color.z * ((gl_Position.z + 1.0) * 0.5);
    frag_color.x = u_color.x * gl_Position.x;
    frag_color.y = u_color.y * gl_Position.y;
    frag_color.z = u_color.z * gl_Position.z;
    frag_color.w = 1.0;
}
