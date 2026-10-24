struct PerInstanceAccessor {
    uint model_idx;
    uint transform_idx;
    uint color_idx;
};

struct PerVertexAccessor {
    uint placeholder;
};

struct ModelHeader {
    uint vertices_offset;
    uint normals_offset;
    uint indices_offset;
};

struct Transform {
    float3 position;
    float4 rotation;
    float3 scale;
};

struct View {
    float2 view_size;
    float2 view_range;
    float3 view_eye;
    float3 view_center;
    float3 view_up;
    float view_fov;
};

StructuredBuffer<PerInstanceAccessor> instance_accessors : register(t0, space0);
StructuredBuffer<PerVertexAccessor>   vertex_accessors   : register(t1, space0);
StructuredBuffer<ModelHeader>         model_headers      : register(t2, space0);
StructuredBuffer<float4>              model_vertices     : register(t3, space0);
StructuredBuffer<float4>              model_normals      : register(t4, space0);
StructuredBuffer<uint4>                model_indices     : register(t5, space0);
StructuredBuffer<Transform>           transforms         : register(t6, space0);
StructuredBuffer<View>                view_buf           : register(t7, space0);

struct VSInput {
    uint instance_id : SV_InstanceID;
    uint vertex_id : SV_VertexID;
};

struct VSOutput {
    float4 position : SV_Position;
    nointerpolation uint instance_idx : TEXCOORD0;
    float3 pos : TEXCOORD1;
    nointerpolation float3 normal : NORMAL0;
};

float4x4 perspective_matrix(float fov, float aspect, float near_z, float far_z) {
    float tan_half_fov = tan(fov / 2);
    return float4x4(
        1 / (aspect * tan_half_fov), 0,                0,                        0,
        0,                           1 / tan_half_fov, 0,                        0,
        0,                           0,                far_z / (far_z - near_z),  -(far_z * near_z) / (far_z - near_z),
        0,                           0,                1,                        0
    );
}

float4x4 look_at_matrix(float3 eye, float3 center, float3 up) {
    float3 f = normalize(center - eye);
    float3 r = normalize(cross(f, up));
    float3 u = cross(r, f);
    float dr = dot(r, eye);
    float du = dot(u, eye);
    float df = dot(f, eye);

    return float4x4(
        r.x, r.y, r.z, -dr,
        u.x, u.y, u.z, -du,
        f.x, f.y, f.z, -df,
        0, 0, 0, 1
    );
}

float4x4 translation_matrix(float3 t) {
    return float4x4(
        1, 0, 0, t.x,
        0, 1, 0, t.y,
        0, 0, 1, t.z,
        0, 0, 0, 1
    );
}

float4x4 rotation_matrix(float4 r) {
    r = normalize(r);
    float x = r.x, y = r.y, z = r.z, w = r.w;
    float xx = x * x, yy = y * y, zz = z * z;
    float xy = x * y, xz = x * z, yz = y * z;
    float wx = w * x, wy = w * y, wz = w * z;

    return float4x4(
        1 - 2 * (yy + zz), 2 * (xy + wz),     2 * (xz - wy),     0,
        2 * (xy - wz),     1 - 2 * (xx + zz), 2 * (yz + wx),     0,
        2 * (xz + wy),     2 * (yz - wx),     1 - 2 * (xx + yy), 0,
        0,                 0,                 0,                 1
    );
}

float4x4 scale_matrix(float3 s) {
    return float4x4(
        s.x, 0,   0,   0,
        0,   s.y, 0,   0,
        0,   0,   s.z, 0,
        0,   0,   0,   1
    );
}

VSOutput main(VSInput input) {
    VSOutput output;

    uint model_idx = instance_accessors[input.instance_id].model_idx;
    uint transform_idx = instance_accessors[input.instance_id].transform_idx;

    ModelHeader model_header = model_headers[model_idx];
    uint vertex_idx = model_indices[model_header.indices_offset + input.vertex_id];
    float3 vertex_pos = model_vertices[model_header.vertices_offset + vertex_idx];
    float3 vertex_normal = model_normals[model_header.normals_offset + vertex_idx];

    float3 position = transforms[transform_idx].position;
    float4 rotation = transforms[transform_idx].rotation;
    float3 scale = transforms[transform_idx].scale;

    View view = view_buf[0];

    float4x4 model_matrix = mul(translation_matrix(position), mul(rotation_matrix(rotation), scale_matrix(scale)));
    float4x4 view_matrix = look_at_matrix(view.view_eye, view.view_center, view.view_up);
    float4x4 proj_matrix = perspective_matrix(view.view_fov, view.view_size.x / view.view_size.y, view.view_range.x, view.view_range.y);

    output.position = mul(proj_matrix, mul(view_matrix, mul(model_matrix, float4(vertex_pos, 1))));

    output.instance_idx = input.instance_id;

    float4 world_pos = mul(model_matrix, float4(vertex_pos, 1));
    output.pos = world_pos.xyz;

    float3x3 normal_matrix = mul((float3x3)rotation_matrix(rotation), (float3x3)scale_matrix(1 / scale));
    output.normal = normalize(mul(normal_matrix, vertex_normal));

    return output;
}