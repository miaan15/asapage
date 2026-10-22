#version 450

layout(set = 1, binding = 0) uniform View {
    vec2 u_screen_size;
    vec3 u_camera_eye;
    vec3 u_camera_center;
    vec3 u_camera_up;
    float u_camera_fov;
};

layout(location = 0) in vec3 i_pos;
layout(location = 1) in vec3 i_normal;

layout(location = 4) in vec3 i_translation;
layout(location = 5) in vec4 i_rotation;
layout(location = 6) in vec3 i_scale;

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec2 o_uv;

mat4 perspective_matrix(float fov, float aspect, float near, float far) {
    float htfov = tan(fov / 2);
    return mat4(
        1 / (aspect * htfov), 0,         0,                            0,
        0,                    1 / htfov, 0,                            0,
        0,                    0,         far / (far - near),           1,
        0,                    0,         -(far * near) / (far - near), 0
    );
}

mat4 look_at_matrix(vec3 eye, vec3 center, vec3 up) {
    vec3 f = normalize(center - eye);
    vec3 r = normalize(cross(f, up));
    vec3 u = cross(r, f);
    return mat4(
        r.x,          u.x,          f.x,          0,
        r.y,          u.y,          f.y,          0,
        r.z,          u.z,          f.z,          0,
        -dot(r, eye), -dot(u, eye), -dot(f, eye), 1
    );
}

mat4 translation_matrix(vec3 t) {
    return mat4(
        1,   0,   0,   0,
        0,   1,   0,   0,
        0,   0,   1,   0,
        t.x, t.y, t.z, 1
    );
}

mat4 rotation_matrix(vec4 r) {
    r = normalize(r);
    float x = r.x, y = r.y, z = r.z, w = r.w;
    return mat4(
        1 - 2 * (y*y + z*z), 2 * (x*y + z*w),     2 * (x*z - y*w),     0,
        2 * (x*y - z*w),     1 - 2 * (x*x + z*z), 2 * (y*z + x*w),     0,
        2 * (x*z + y*w),     2 * (y*z - x*w),     1 - 2 * (x*x + y*y), 0,
        0,                   0,                   0,                   1
    );
}

mat4 scale_matrix(vec3 s) {
    return mat4(
        s.x, 0,   0,   0,
        0,   s.y, 0,   0,
        0,   0,   s.z, 0,
        0,   0,   0,   1
    );
}

void main() {
    mat4 model = translation_matrix(i_translation) *
                 rotation_matrix(i_rotation) *
                 scale_matrix(i_scale);

    mat4 view = look_at_matrix(u_camera_eye, u_camera_center, u_camera_up);
    mat4 proj = perspective_matrix(u_camera_fov, u_screen_size.x / u_screen_size.y, 0.1, 100.0);

    gl_Position = proj * view * model * vec4(i_pos, 1.0);

    vec3 light_dir = normalize(u_camera_center - u_camera_eye);
    float cos_angle = 2 * dot(i_normal, -light_dir);

    // vec3 color = (i_pos + vec3(1)) / 2 * 0.8;
    // color += pow(max(0, cos_angle * 0.1), 0.5);
    // o_color = vec4(color, 1);
    o_uv = (i_pos.xy + vec2(1)) / 2;

    o_color = vec4(o_uv, 0, 1);
}
