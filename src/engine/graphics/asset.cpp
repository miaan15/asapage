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

// FIXME this should be later more generic to support custom shader
struct Material {
    Shader shader;
    Texture texture;
    Color color;

    u64 data[4];
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
constexpr const char *SHADER_VERTEX_DEFAULT_PATH = "shader/default.vs";
constexpr const char *SHADER_FRAGMENT_DEFAULT_PATH = "shader/default.fs";

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

    { // texture
        // stub // TODO
        list_create(&graphics_asset_sys.texture_list);

        // default
        Texture *new_texture;

        list_create(&graphics_asset_sys.texture_list, (Texture*)nullptr, &new_texture);
        *new_texture = LoadTextureFromImage(GenImageColor(1, 1, WHITE));
        graphics_asset_sys.texture_num++;
    }

    { // shader
        // stub // TODO
        list_create(&graphics_asset_sys.shader_list);

        // default
        Shader *new_shader;

        list_create(&graphics_asset_sys.shader_list, (Shader*)nullptr, &new_shader);
        *new_shader = LoadShader(file_source_path(SHADER_VERTEX_DEFAULT_PATH).c_str(), file_source_path(SHADER_FRAGMENT_DEFAULT_PATH).c_str());
        graphics_asset_sys.shader_num++;
    }

    { // material
        // stub // TODO
        list_create(&graphics_asset_sys.material_list);

        // default
        Material *new_material;

        list_create(&graphics_asset_sys.material_list, (Material*)nullptr, &new_material);
        new_material->shader = *list_get(&graphics_asset_sys.shader_list, SHADER_DEFAULT_KEY.idx);
        new_material->texture = *list_get(&graphics_asset_sys.texture_list, TEXTURE_DEFAULT_KEY.idx);
        new_material->color = WHITE;
        graphics_asset_sys.material_num++;
    }

    { // mesh
        // stub // TODO
        list_create(&graphics_asset_sys.mesh_list);

        // default
        Mesh meshes[] = {
            GenMeshCube(1.0, 1.0, 1.0),
            GenMeshSphere(0.5, 16, 16),
            GenMeshPlane(1.0, 1.0, 1, 1),
            GenMeshCylinder(0.5, 1.0, 16),
            GenMeshCone(0.5, 1.0, 16),
            GenMeshTorus(0.25, 1.0, 16, 16),
            GenMeshHemiSphere(0.5, 16, 16),
        };
        Mesh *new_mesh;
        for (i32 i = 0; i < (i32)(sizeof(meshes) / sizeof(meshes[0])); i++) {
            list_create(&graphics_asset_sys.mesh_list, (Mesh*)nullptr, &new_mesh);
            *new_mesh = meshes[i];
            graphics_asset_sys.mesh_num++;
        }
    }
}

TextureKey texture_create(const char *path, Texture **out_texture = nullptr) {
    Texture *new_texture;
    i32 idx = list_create(&graphics_asset_sys.texture_list, (Texture*)nullptr, &new_texture);
    *new_texture = LoadTexture(path);

    graphics_asset_sys.texture_num++;

    if (out_texture != nullptr) { *out_texture = new_texture; }

    log_debug("Created Texture [%d]: path = [%s]; size = [%d, %d]", idx, path, new_texture->width, new_texture->height);

    return TextureKey{ .idx = idx };
}

ShaderKey shader_create(const char *vs_path, const char *fs_path, Shader **out_shader = nullptr) {
    Shader *new_shader;
    i32 idx = list_create(&graphics_asset_sys.shader_list, (Shader *)nullptr, &new_shader);
    *new_shader = LoadShader(vs_path, fs_path);

    graphics_asset_sys.shader_num++;

    if (out_shader != nullptr) { *out_shader = new_shader; }

    log_debug("Created Shader [%d]: vertex_path = [%s]; fragment_path = [%s]", idx, vs_path, fs_path);

    return ShaderKey{ .idx = idx };
}

MaterialKey material_create(ShaderKey shader_key = SHADER_DEFAULT_KEY,
                            TextureKey texture_key = TEXTURE_DEFAULT_KEY,
                            Material **out_material = nullptr) {
    Material *new_material;
    i32 idx = list_create(&graphics_asset_sys.material_list, (Material *)nullptr, &new_material);
    new_material->shader = *list_get(&graphics_asset_sys.shader_list, shader_key.idx);
    new_material->texture = *list_get(&graphics_asset_sys.texture_list, texture_key.idx);
    new_material->color = WHITE;

    graphics_asset_sys.material_num++;

    if (out_material != nullptr) { *out_material = new_material; }

    log_debug("Created Material [%d]: shader = [%d]", idx, shader_key.idx);

    return MaterialKey{ .idx = idx };
}

struct ModelLoadResult {
    MeshKey *mesh_keys; Mesh **meshs; i32 mesh_num;
};
ModelLoadResult model_load(const char *path, Arena *arena = tick_arena_ptr) {
    Model model = LoadModel(path);

    ModelLoadResult result = {};
    result.mesh_num = model.meshCount;

    if (result.mesh_num > 0) {
        result.mesh_keys = (MeshKey *)arena_alloc(arena, sizeof(MeshKey) * result.mesh_num);
        result.meshs = (Mesh **)arena_alloc(arena, sizeof(Mesh *) * result.mesh_num);
        for (i32 i = 0; i < result.mesh_num; i++) {
            Mesh *new_mesh;
            i32 idx = list_create(&graphics_asset_sys.mesh_list, (Mesh *)nullptr, &new_mesh);
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

inline Texture* TEXTURE_STUB()      { return texture_get(TEXTURE_STUB_KEY); }
inline Texture* TEXTURE_DEFAULT()   { return texture_get(TEXTURE_DEFAULT_KEY); }

inline Shader* SHADER_STUB()        { return shader_get(SHADER_STUB_KEY); }
inline Shader* SHADER_DEFAULT()     { return shader_get(SHADER_DEFAULT_KEY); }

inline Material* MATERIAL_STUB()    { return material_get(MATERIAL_STUB_KEY); }
inline Material* MATERIAL_DEFAULT() { return material_get(MATERIAL_DEFAULT_KEY); }

inline Mesh* MESH_STUB()            { return mesh_get(MESH_STUB_KEY); }
inline Mesh* MESH_CUBE()            { return mesh_get(MESH_CUBE_KEY); }
inline Mesh* MESH_SPHERE()          { return mesh_get(MESH_SPHERE_KEY); }
inline Mesh* MESH_PLANE()           { return mesh_get(MESH_PLANE_KEY); }
inline Mesh* MESH_CYLINDER()        { return mesh_get(MESH_CYLINDER_KEY); }
inline Mesh* MESH_CONE()            { return mesh_get(MESH_CONE_KEY); }
inline Mesh* MESH_TORUS()           { return mesh_get(MESH_TORUS_KEY); }
inline Mesh* MESH_HEMISPHERE()      { return mesh_get(MESH_HEMISPHERE_KEY); }

}
