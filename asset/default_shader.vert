#version 450

// per instance
struct PerInstanceAccessor {
    uint model_idx;
    uint object_idx;
};
layout(std430, set = 0, binding = 0) readonly buffer PerInstanceAccessorBuf {
    PerInstanceAccessor instance_accessor_buf[];
};

// per vertex
struct PerVertexAccessor {
    uint placeholder;
};
layout(std430, set = 0, binding = 1) readonly buffer PerVertexAccessorBuf {
    PerVertexAccessor vertex_accessor_buf[];
};

// model
struct ModelAccessor {
    uint vertex_offset;
    uint index_offset;
};
layout(std430, set = 0, binding = 2) readonly buffer ModelAccessorBuf {
    ModelAccessor model_accessor_buf[];
};

// actual model data
struct ModelVertex {
    vec3 pos;
    vec3 normal;
    vec2 uv;
};
layout(std430, set = 0, binding = 3) readonly buffer ModelVertexBuf {
    ModelVertex model_vertex_buf[];
};

layout(std430, set = 0, binding = 4) readonly buffer ModelIndexBuf {
    uint model_index_buf[];
};

// object
// transform
layout(std430, set = 0, binding = 5) readonly buffer TransformBuf {
    mat4 transform_buf[];
};

struct ObjectData {
    vec3 color;
};
layout(std430, set = 0, binding = 6) readonly buffer ObjectDataBuf {
    ObjectData object_data_buf[];
};

// global
layout(std430, set = 0, binding = 7) readonly buffer ViewBuf {
    vec2 view_size;
    vec2 view_range;
    vec3 view_eye;
    vec3 view_center;
    vec3 view_up;
    float view_fov;
};

layout(location = 0) out vec3 o_pos;
layout(location = 1) flat out vec3 o_normal;
layout(location = 2) out vec2 o_uv;
layout(location = 3) out vec3 o_color;

mat4 perspective_matrix(float fov, float aspect, float near, float far);
mat4 look_at_matrix(vec3 eye, vec3 center, vec3 up);
mat4 translation_matrix(vec3 t);
mat4 rotation_matrix(vec4 r);
mat4 scale_matrix(vec3 s);

void main() {
    uint model_idx = instance_accessor_buf[gl_InstanceIndex].model_idx;
    uint object_idx = instance_accessor_buf[gl_InstanceIndex].object_idx;

    // model
    ModelAccessor model_accessor = model_accessor_buf[model_idx];
    uint vertex_idx = model_index_buf[model_accessor.index_offset + gl_VertexIndex];
    ModelVertex vertex_data = model_vertex_buf[model_accessor.vertex_offset + vertex_idx];

    // object
    mat4 transform = transform_buf[object_idx];
    ObjectData object_data = object_data_buf[object_idx];

    //
    mat4 view_matrix = look_at_matrix(view_eye, view_center, view_up);
    mat4 proj_matrix = perspective_matrix(view_fov, view_size.x / view_size.y, view_range.x, view_range.y);

    vec4 world_pos = transform * vec4(vertex_data.pos, 1.0);

    //
    gl_Position = proj_matrix * view_matrix * world_pos;

    //
    o_pos = world_pos.xyz;

    mat3 normal_matrix = mat3(transpose(inverse(transform)));
    o_normal = normalize(normal_matrix * vertex_data.normal);

    o_uv = vertex_data.uv;

    o_color = object_data.color;
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
