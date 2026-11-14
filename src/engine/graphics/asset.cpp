module;

#include <raylib.h>

export module graphics_asset;

import define;
import mem;
import internal;
import log;
import math;
import file;
import list;

export namespace asapage {

using RlTexture = ::Texture;
using RlShader = ::Shader;
using RlMaterial = ::Material;
using RlMesh = ::Mesh;

struct TextureKey {
    union { u64 raw; struct {
        i32 idx;
        u32 _reserve; };
    };
};

struct ShaderKey {
    union { u64 raw; struct {
        i32 idx;
        u32 _reserve; };
    };
};

struct MaterialKey {
    union { u64 raw; struct {
        i32 idx;
        u32 _reserve; };
    };
};

struct MeshKey {
    union { u64 raw; struct {
        i32 idx;
        u32 _reserve; };
    };
};

struct Material {
    ShaderKey shader_key;
    u64 data[16];
};

// default materials
struct ShaderUnlitData {
    TextureKey texture_key;
    Vec3 color;
};

struct ShaderLitData {
    TextureKey texture_key;
    Vec3 albedo;
    f32 metallic;
    f32 smoothness;
};

constexpr i32 TEXTURE_CAP_DEFAULT  = 512;
constexpr i32 SHADER_CAP_DEFAULT   = 128;
constexpr i32 MATERIAL_CAP_DEFAULT = 512;
constexpr i32 MESH_CAP_DEFAULT     = 512;
constexpr i32 OBJECT_CAP_DEFAULT   = 100'000; // 100k

constexpr TextureKey TEXTURE_STUB_KEY = { .idx = 0 };
constexpr ShaderKey SHADER_STUB_KEY = { .idx = 0 };
constexpr MaterialKey MATERIAL_STUB_KEY = { .idx = 0 };
constexpr MeshKey MESH_STUB_KEY = { .idx = 0 };

constexpr TextureKey TEXTURE_DEFAULT_KEY = { .idx = 1 };

constexpr ShaderKey SHADER_UNLIT_KEY = { .idx = 1 };
constexpr const char *SHADER_UNLIT_VERTEX_PATH = "shader/unlit.vs";
constexpr const char *SHADER_UNLIT_FRAGMENT_PATH = "shader/unlit.fs";
constexpr ShaderKey SHADER_LIT_KEY = { .idx = 2 };
constexpr const char *SHADER_LIT_VERTEX_PATH = "shader/lit.vs";
constexpr const char *SHADER_LIT_FRAGMENT_PATH = "shader/lit.fs";

constexpr MaterialKey MATERIAL_UNLIT_DEFAULT_KEY = { .idx = 1 };
constexpr MaterialKey MATERIAL_LIT_DEFAULT_KEY = { .idx = 2 };

constexpr MeshKey MESH_CUBE_KEY = { .idx = 1 };
constexpr MeshKey MESH_SPHERE_KEY = { .idx = 2 };
constexpr MeshKey MESH_PLANE_KEY = { .idx = 3 };
constexpr MeshKey MESH_CYLINDER_KEY = { .idx = 4 };
constexpr MeshKey MESH_CONE_KEY = { .idx = 5 };
constexpr MeshKey MESH_TORUS_KEY = { .idx = 6 };
constexpr MeshKey MESH_HEMISPHERE_KEY = { .idx = 7 };

struct GraphicsAssetSys {
    List<RlTexture> texture_list;
    i32 texture_num;

    List<RlShader> shader_list;
    i32 shader_num;

    List<Material> material_list;
    i32 material_num;

    List<RlMesh> mesh_list;
    i32 mesh_num;
};
inline GraphicsAssetSys graphics_asset_sys = {};

void graphics_asset_sys_init(i32 texture_cap, i32 shader_cap, i32 material_cap, i32 mesh_cap) {
    list_init(&graphics_asset_sys.texture_list, omni_arena_ptr, texture_cap);
    list_init(&graphics_asset_sys.shader_list, omni_arena_ptr, shader_cap);
    list_init(&graphics_asset_sys.material_list, omni_arena_ptr, material_cap);
    list_init(&graphics_asset_sys.mesh_list, omni_arena_ptr, mesh_cap);

    // stub
    LIST_STUB(&graphics_asset_sys.texture_list); // TODO make it caro blackpink

    // texture default
    Image default_image = GenImageColor(1, 1, WHITE);
    RlTexture default_texture = LoadTextureFromImage(default_image);
    UnloadImage(default_image);
    list_create(&graphics_asset_sys.texture_list, &default_texture);
    graphics_asset_sys.texture_num++;

    // shader defaults
    RlShader unlit_shader = LoadShader(file_source_path(SHADER_UNLIT_VERTEX_PATH).c_str(),
                                       file_source_path(SHADER_UNLIT_FRAGMENT_PATH).c_str());
    list_create(&graphics_asset_sys.shader_list, &unlit_shader);
    graphics_asset_sys.shader_num++;

    RlShader lit_shader = LoadShader(file_source_path(SHADER_LIT_VERTEX_PATH).c_str(),
                                     file_source_path(SHADER_LIT_FRAGMENT_PATH).c_str());
    list_create(&graphics_asset_sys.shader_list, &lit_shader);
    graphics_asset_sys.shader_num++;

    // material defaults
    Material unlit_material = { .shader_key = SHADER_UNLIT_KEY };
    ShaderUnlitData unlit_data = { .texture_key = TEXTURE_DEFAULT_KEY, .color = VEC3_ONE };
    mem_copy(unlit_material.data, &unlit_data, sizeof(ShaderUnlitData));
    list_create(&graphics_asset_sys.material_list, &unlit_material);
    graphics_asset_sys.material_num++;

    Material lit_material = { .shader_key = SHADER_LIT_KEY };
    ShaderLitData lit_data = { .texture_key = TEXTURE_DEFAULT_KEY, .albedo = VEC3_ONE, .metallic = 0.0f, .smoothness = 0.5f };
    mem_copy(lit_material.data, &lit_data, sizeof(ShaderLitData));
    list_create(&graphics_asset_sys.material_list, &lit_material);
    graphics_asset_sys.material_num++;

    // mesh defaults
    RlMesh default_meshes[] = {
        GenMeshCube(1.0f, 1.0f, 1.0f),
        GenMeshSphere(0.5f, 16, 16),
        GenMeshPlane(1.0f, 1.0f, 1, 1),
        GenMeshCylinder(0.5f, 1.0f, 16),
        GenMeshCone(0.5f, 1.0f, 16),
        GenMeshTorus(0.25f, 1.0f, 16, 16),
        GenMeshHemiSphere(0.5f, 16, 16),
    };
    for (i32 i = 0; i < (i32)(sizeof(default_meshes) / sizeof(default_meshes[0])); i++) {
        list_create(&graphics_asset_sys.mesh_list, &default_meshes[i]);
        graphics_asset_sys.mesh_num++;
    }
}

TextureKey texture_create(const char *path, RlTexture **out_texture = nullptr) {
    RlTexture *new_texture;
    i32 idx = list_create(&graphics_asset_sys.texture_list, (RlTexture*)nullptr, &new_texture);
    *new_texture = LoadTexture(path);

    graphics_asset_sys.texture_num++;

    if (out_texture != nullptr) { *out_texture = new_texture; }

    log_debug("Created Texture [%d]: path = [%s]; size = [%d, %d]", idx, path, new_texture->width, new_texture->height);

    return TextureKey{ .idx = idx };
}

ShaderKey shader_create(const char *vs_path, const char *fs_path, RlShader **out_shader = nullptr) {
    RlShader *new_shader;
    i32 idx = list_create(&graphics_asset_sys.shader_list, (RlShader *)nullptr, &new_shader);
    *new_shader = LoadShader(vs_path, fs_path);

    graphics_asset_sys.shader_num++;

    if (out_shader != nullptr) { *out_shader = new_shader; }

    log_debug("Created Shader [%d]: vertex_path = [%s]; fragment_path = [%s]", idx, vs_path, fs_path);

    return ShaderKey{ .idx = idx };
}

MaterialKey material_create(ShaderKey shader_key = SHADER_UNLIT_KEY,
                            const void *data = nullptr,
                            Material **out_material = nullptr) {
    Material *new_material;
    i32 idx = list_create(&graphics_asset_sys.material_list, (Material *)nullptr, &new_material);
    new_material->shader_key = shader_key;
    if (data != nullptr) {
        mem_copy(new_material->data, data, sizeof(new_material->data));
    } else {
        mem_zero(new_material->data, sizeof(new_material->data));
    }

    graphics_asset_sys.material_num++;

    if (out_material != nullptr) { *out_material = new_material; }

    log_debug("Created Material [%d]: shader = [%d]", idx, shader_key.idx);

    return MaterialKey{ .idx = idx };
}

struct ModelLoadResult {
    MeshKey *mesh_keys; RlMesh **meshs; i32 mesh_num;
};
ModelLoadResult model_load(const char *path, Arena *arena = tick_arena_ptr) {
    Model model = LoadModel(path);

    ModelLoadResult result = {};
    result.mesh_num = model.meshCount;

    if (result.mesh_num > 0) {
        result.mesh_keys = (MeshKey *)arena_alloc(arena, sizeof(MeshKey) * result.mesh_num);
        result.meshs = (RlMesh **)arena_alloc(arena, sizeof(RlMesh *) * result.mesh_num);
        for (i32 i = 0; i < result.mesh_num; i++) {
            RlMesh *new_mesh;
            i32 idx = list_create(&graphics_asset_sys.mesh_list, (RlMesh *)nullptr, &new_mesh);
            *new_mesh = model.meshes[i];

            graphics_asset_sys.mesh_num++;

            log_debug(".Loaded Mesh [%d] from model: path = [%s]; vertex_num = [%d]; triangle_num = [%d]", idx, path, new_mesh->vertexCount, new_mesh->triangleCount);

            result.mesh_keys[i] = MeshKey{ .idx = idx };
            result.meshs[i] = new_mesh;
        }
    }

    log_debug("Loaded Model: path = [%s]; mesh_num = [%d]", path, result.mesh_num);

    return result;
}

RlTexture *texture_get(TextureKey key) {
    if (key.idx < 0 || key.idx > graphics_asset_sys.texture_num) {
        log_err("texture_get(): try to get with key has idx out of bounds -> stub: key = [%d]", key.idx);
        return LIST_STUB(&graphics_asset_sys.texture_list);
    }
    return list_get(&graphics_asset_sys.texture_list, key.idx);
}

RlShader *shader_get(ShaderKey key) {
    if (key.idx < 0 || key.idx > graphics_asset_sys.shader_num) {
        log_err("shader_get(): try to get with key has idx out of bounds -> stub: key = [%d]", key.idx);
        return LIST_STUB(&graphics_asset_sys.shader_list);
    }
    return list_get(&graphics_asset_sys.shader_list, key.idx);
}

Material *material_get(MaterialKey key) {
    if (key.idx < 0 || key.idx > graphics_asset_sys.material_num) {
        log_err("material_get(): try to get with key has idx out of bounds -> stub: key = [%d]", key.idx);
        return LIST_STUB(&graphics_asset_sys.material_list);
    }
    return list_get(&graphics_asset_sys.material_list, key.idx);
}

RlMesh *mesh_get(MeshKey key) {
    if (key.idx < 0 || key.idx > graphics_asset_sys.mesh_num) {
        log_err("mesh_get(): try to get with key has idx out of bounds -> stub: key = [%d]", key.idx);
        return LIST_STUB(&graphics_asset_sys.mesh_list);
    }
    return list_get(&graphics_asset_sys.mesh_list, key.idx);
}

}
