#version 450

// per instance
struct PerInstanceAccessor {
    uint model_idx;
    uint transform_idx;
    uint color_idx;
};
layout(set = 0, binding = 0) readonly buffer PerInstanceAccessorBuf {
    PerInstanceAccessor instance_accessors[];
};

// per vertex
struct PerVertexAccessor {
    uint placeholder;
};
layout(set = 0, binding = 1) readonly buffer PerVertexAccessorBuf {
    PerVertexAccessor vertex_accessors[];
};

// model
struct ModelHeader {
    uint vertices_offset;
    uint normals_offset;
    uint indices_offset;
};
layout(set = 0, binding = 2) readonly buffer ModelHeaderBuf {
    ModelHeader model_headers[];
};

layout(set = 0, binding = 3) readonly buffer ModelVertexBuf {
    vec3 model_vertices[];
};
layout(set = 0, binding = 4) readonly buffer ModelNormalBuf {
    vec3 model_normals[];
};
layout(set = 0, binding = 5) readonly buffer ModelIndexBuf {
    uint model_indices[];
};

// transform
struct Transform {
    vec3 position;
    vec4 rotation;
    vec3 scale;
};
layout(set = 0, binding = 6) readonly buffer TransformBuf {
    Transform transforms[];
};

// view
layout(set = 0, binding = 7) readonly buffer ViewBuf {
    vec2 view_size;
    vec2 view_range;
    vec3 view_eye;
    vec3 view_center;
    vec3 view_up;
    float view_fov;
};

layout(location = 0) flat out uint o_instance_idx;
layout(location = 1) out vec3 o_pos;
layout(location = 2) flat out vec3 fo_normal;

mat4 perspective_matrix(float fov, float aspect, float near, float far);
mat4 look_at_matrix(vec3 eye, vec3 center, vec3 up);
mat4 translation_matrix(vec3 t);
mat4 rotation_matrix(vec4 r);
mat4 scale_matrix(vec3 s);

void main() {
    uint model_idx = instance_accessors[gl_InstanceIndex].model_idx;
    uint transform_idx = instance_accessors[gl_InstanceIndex].transform_idx;

    // model
    ModelHeader model_header = model_headers[model_idx];
    uint vertex_idx = model_indices[model_header.indices_offset + gl_VertexIndex];
    vec3 vertex_pos = model_vertices[model_header.vertices_offset + vertex_idx].xyz;
    vec3 vertex_normal = model_normals[model_header.normals_offset + vertex_idx].xyz;

    // transform
    vec3 position = transforms[transform_idx].position;
    vec4 rotation = transforms[transform_idx].rotation;
    vec3 scale = transforms[transform_idx].scale;

    mat4 model = translation_matrix(position) * rotation_matrix(rotation) * scale_matrix(scale);
    mat4 view = look_at_matrix(view_eye, view_center, view_up);
    mat4 proj = perspective_matrix(view_fov, view_size.x / view_size.y, view_range.x, view_range.y);

    gl_Position = proj * view * model * vec4(vertex_pos, 1);

    o_instance_idx = gl_InstanceIndex;

    vec4 world_pos = model * vec4(vertex_pos, 1);
    o_pos = world_pos.xyz;

    mat3 normal_matrix = mat3(rotation_matrix(rotation)) * mat3(scale_matrix(1.0 / scale));
    fo_normal = normalize(normal_matrix * vertex_normal);
}

mat4 perspective_matrix(float fov, float aspect, float near, float far) {
    float tan_half_fov = tan(fov / 2);
    mat4 res = mat4(0);
    res[0][0] = 1 / (aspect * tan_half_fov);
    res[1][1] = 1 / tan_half_fov;
    res[2][2] = far / (far - near);
    res[3][2] = -(far * near) / (far - near);
    res[2][3] = 1;
    return res;
}

mat4 look_at_matrix(vec3 eye, vec3 center, vec3 up) {
    vec3 f = normalize(center - eye);
    vec3 r = normalize(cross(f, up));
    vec3 u = cross(r, f);
    float dr = dot(r, eye), du = dot(u, eye), df = dot(f, eye);
    return mat4(
        r.x, u.x, f.x, 0,
        r.y, u.y, f.y, 0,
        r.z, u.z, f.z, 0,
        -dr, -du, -df, 1
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
    float xx = r.x * r.x, yy = r.y * r.y, zz = r.z * r.z;
    float xy = r.x * r.y, xz = r.x * r.z, yz = r.y * r.z;
    float wx = r.w * r.x, wy = r.w * r.y, wz = r.w * r.z;
    return mat4(
        1 - 2 * (yy + zz),     2 * (xy + wz),     2 * (xz - wy), 0,
            2 * (xy - wz), 1 - 2 * (xx + zz),     2 * (yz + wx), 0,
            2 * (xz + wy),     2 * (yz - wx), 1 - 2 * (xx + yy), 0,
                        0,                 0,                 0, 1
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
