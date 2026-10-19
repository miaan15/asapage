#version 450

layout(location = 0) in vec3 i_pos;

layout(set = 1, binding = 0) uniform MVP {
    vec2 u_screen_size;

    float u_focal_length;

    vec3 u_translation;
    vec4 u_rotation;
    vec3 u_scale;
};

layout(set = 1, binding = 3) uniform Color {
    vec3 u_color;
};

layout(location = 0) out vec4 frag_color;

mat4 get_viewport_matrix(vec2 screen_size) {
    float w = screen_size.x, h = screen_size.y;
    return mat4(
        2/w, 0,   0, 0,
        0,   2/h, 0, 0,
        0,   0,   1, 0,
        0,   0,   0, 1
    );
}

mat4 get_perspective_matrix(float focal_length) {
    float f = focal_length;
    return mat4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, -1/f,
        0, 0, 0, 1
    );
}

mat4 get_translation_matrix(vec3 translation) {
    float x = translation.x, y = translation.y, z = translation.z;
    return mat4(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        x, y, z, 1
    );
}

mat4 get_rotation_matrix(vec4 rotation) {
    rotation = normalize(rotation);
    float x = rotation.x, y = rotation.y, z = rotation.z, w = rotation.w;
    return mat4(
        1 - 2*(y*y + z*z), 2*(x*y + z*w),     2*(x*z - y*w),     0,
        2*(x*y - z*w),     1 - 2*(x*x + z*z), 2*(y*z + x*w),     0,
        2*(x*z + y*w),     2*(y*z - x*w),     1 - 2*(x*x + y*y), 0,
        0,                 0,                 0,                 1
    );
}

mat4 get_scale_matrix(vec3 scale) {
    float x = scale.x, y = scale.y, z = scale.z;
    return mat4(
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1
    );
}

void main() {
    mat4 transform_matrix = get_translation_matrix(u_translation) *
                            get_rotation_matrix(u_rotation) *
                            get_scale_matrix(u_scale);

    mat4 model_view_matrix = transform_matrix;

    mat4 viewport_matrix = get_viewport_matrix(u_screen_size);

    mat4 perspective_matrix = get_perspective_matrix(u_focal_length);

    gl_Position = perspective_matrix *
                  viewport_matrix *
                  model_view_matrix *
                  vec4(i_pos, 1);

    frag_color.x = u_color.x * (i_pos.x + 1) * 0.5 * (gl_Position.z + 1) * 0.5;
    frag_color.y = u_color.y * (i_pos.y + 1) * 0.5 * (gl_Position.z + 1) * 0.5;
    frag_color.z = u_color.z * (i_pos.z + 1) * 0.5 * (gl_Position.z + 1) * 0.5;
    frag_color.w = 1;
}
