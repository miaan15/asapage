module;

#include <raylib.h>

export module graphics_asset;

import define;
import mem;
import internal;
import log;
import math;
import list;

export namespace asapage {

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
    Vec4 color;
    TextureKey texture_key;
    ShaderKey shader_key;

    f32 params[4]; // reserve what raylib provides
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
constexpr ShaderKey SHADER_DEFAULT_KEY = { .idx = 1 };
constexpr MaterialKey MATERIAL_DEFAULT_KEY = { .idx = 1 };

constexpr MeshKey MESH_CUBE_KEY = { .idx = 1 };
constexpr MeshKey MESH_SPHERE_KEY = { .idx = 2 };
constexpr MeshKey MESH_PLANE_KEY = { .idx = 3 };
constexpr MeshKey MESH_CYLINDER_KEY = { .idx = 4 };
constexpr MeshKey MESH_CONE_KEY = { .idx = 5 };
constexpr MeshKey MESH_TORUS_KEY = { .idx = 6 };
constexpr MeshKey MESH_HEMISPHERE_KEY = { .idx = 7 };

struct GraphicsAssetSys {
    List<Texture> texture_list;
    i32 texture_num;

    List<Shader> shader_list;
    i32 shader_num;

    List<Material> material_list;
    i32 material_num;

    List<Mesh> mesh_list;
    i32 mesh_num;
};
inline GraphicsAssetSys graphics_asset_sys = {};

void graphics_asset_sys_init(i32 texture_cap, i32 shader_cap, i32 material_cap, i32 mesh_cap) {
    list_init(&graphics_asset_sys.texture_list, omni_arena_ptr, texture_cap);
    list_init(&graphics_asset_sys.shader_list, omni_arena_ptr, shader_cap);
    list_init(&graphics_asset_sys.material_list, omni_arena_ptr, material_cap);
    list_init(&graphics_asset_sys.mesh_list, omni_arena_ptr, mesh_cap);
}

TextureKey texture_create(const char *path, Texture *out_texture = nullptr) {
    Texture new_texture = LoadTexture(path);

    i32 idx = list_create(&graphics_asset_sys.texture_list, &new_texture, &out_texture);
    graphics_asset_sys.texture_num++;

    log_debug("Created Texture [%d]: path = [%s]; size = [%d, %d]", idx, path, new_texture.width, new_texture.height);

    return TextureKey{ .idx = idx };
}

ShaderKey shader_create(const char *vs_path, const char *fs_path, Shader *out_shader = nullptr) {
    Shader new_shader = LoadShader(vs_path, fs_path);

    i32 idx = list_create(&graphics_asset_sys.shader_list, &new_shader, &out_shader);
    graphics_asset_sys.shader_num++;

    log_debug("Created Shader [%d]: vertex_path = [%s]; fragment_path = [%s]", idx, vs_path, fs_path);

    return ShaderKey{ .idx = idx };
}

MaterialKey material_create(Vec4 color = VEC4_ONE,
                            TextureKey texture_key = TEXTURE_DEFAULT_KEY,
                            ShaderKey shader_key = SHADER_DEFAULT_KEY,
                            Material *out_material = nullptr) {
    Material new_material = { .color = color,
                              .texture_key = texture_key,
                              .shader_key = shader_key };

    i32 idx = list_create(&graphics_asset_sys.material_list, &new_material, &out_material);
    graphics_asset_sys.material_num++;

    log_debug("Created Material [%d]: color = [%.1f, %.1f, %.1f, %.1f]; texture = [%d]; shader = [%d]",
              idx, color.x, color.y, color.z, color.w, texture_key.idx, shader_key.idx);

    return MaterialKey{ .idx = idx };
}

struct ModelLoadResult {
    MeshKey *mesh_keys; Mesh *meshs; i32 mesh_num;
};
ModelLoadResult model_load(const char *path, Arena *arena = tick_arena_ptr) {
    Model model = LoadModel(path);

    ModelLoadResult result = {};
    result.mesh_num = model.meshCount;

    if (result.mesh_num > 0) {
        result.mesh_keys = (MeshKey *)arena_alloc(omni_arena_ptr, sizeof(MeshKey) * result.mesh_num);
        result.meshs = (Mesh *)arena_alloc(omni_arena_ptr, sizeof(Mesh) * result.mesh_num);
        for (i32 i = 0; i < result.mesh_num; i++) {
            Mesh new_mesh = model.meshes[i];
            i32 idx = list_create(&graphics_asset_sys.mesh_list, &new_mesh);
            graphics_asset_sys.mesh_num++;

            log_debug(".Loaded Mesh [%d] from model: path = [%s]; vertex_num = [%d]; triangle_num = [%d]", idx, path, new_mesh.vertexCount, new_mesh.triangleCount);

            result.mesh_keys[i] = (MeshKey){ .idx = idx };
            result.meshs[i] = new_mesh;
        }
    }

    log_debug("Loaded Model: path = [%s]; mesh_num = [%d]", path, result.mesh_num);

    return result;
}

Texture *texture_get(TextureKey key) {
    if (key.idx < 0 || key.idx > graphics_asset_sys.texture_num) {
        log_err("texture_get(): try to get with key has idx out of bounds -> stub: key = [%d]", key.idx);
        return LIST_STUB(&graphics_asset_sys.texture_list);
    }
    return list_get(&graphics_asset_sys.texture_list, key.idx);
}

Shader *shader_get(ShaderKey key) {
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

Mesh *mesh_get(MeshKey key) {
    if (key.idx < 0 || key.idx > graphics_asset_sys.mesh_num) {
        log_err("mesh_get(): try to get with key has idx out of bounds -> stub: key = [%d]", key.idx);
        return LIST_STUB(&graphics_asset_sys.mesh_list);
    }
    return list_get(&graphics_asset_sys.mesh_list, key.idx);
}

}
