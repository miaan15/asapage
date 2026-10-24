struct PerInstanceAccessor {
    uint model_idx;
    uint transform_idx;
    uint color_idx;
};

struct PerVertexAccessor {
    uint placeholder;
};

struct Color {
    float3 color;
};

struct EnvData {
    float3 env_light_pos;
    float env_light_intensity;
};

struct View {
    float2 view_size;
    float2 view_range;
    float3 view_eye;
    float3 view_center;
    float3 view_up;
    float view_fov;
};

StructuredBuffer<PerInstanceAccessor> instance_accessors : register(t0, space2);
StructuredBuffer<PerVertexAccessor>   vertex_accessors   : register(t1, space2);
StructuredBuffer<Color>               colors             : register(t2, space2);
StructuredBuffer<EnvData>             env_buf            : register(t3, space2);
StructuredBuffer<View>                view_buf           : register(t4, space2);

struct PSInput {
    float4 position : SV_Position;
    nointerpolation uint instance_idx : TEXCOORD0;
    float3 pos : TEXCOORD1;
    nointerpolation float3 normal : NORMAL0;
};

float4 main(PSInput input) : SV_Target0 {
    uint color_idx = instance_accessors[input.instance_idx].color_idx;
    float3 albedo = colors[color_idx].color;

    EnvData env = env_buf[0];
    View view = view_buf[0];

    float3 light_dir = normalize(env.env_light_pos - input.pos);
    float3 view_dir = normalize(view.view_eye - input.pos);
    float3 reflect_light_dir = reflect(-light_dir, input.normal);

    float ambient_strength = 0.1;
    float3 ambient = ambient_strength * albedo;

    float diff = max(dot(input.normal, light_dir), 0);
    float3 diffuse = diff * albedo * 1;

    float shininess = 10;
    float spec = pow(max(dot(reflect_light_dir, view_dir), 0), shininess);
    float3 specular = float3(1, 1, 1) * spec * 0.5;

    float3 final_color = ambient + (diffuse + specular) * env.env_light_intensity;
    return float4(final_color, 1);
}