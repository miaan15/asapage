module;

#include <cassert>
#include <raylib.h>

export module graphics;

export import graphics_asset;
export import graphics_object;

import define;
import mem;
import internal;
import math;
import log;
import list;

export namespace asapage {

struct GraphicsSysDesc {
    i32 texture_cap;
    i32 shader_cap;
    i32 material_cap;
    i32 mesh_cap;
    i32 object_cap;
    Camera3D camera;
};

constexpr GraphicsSysDesc GRAPHICS_SYS_DESC_DEFAULT = {
    .texture_cap  = TEXTURE_CAP_DEFAULT,
    .shader_cap   = SHADER_CAP_DEFAULT,
    .material_cap = MATERIAL_CAP_DEFAULT,
    .mesh_cap     = MESH_CAP_DEFAULT,
    .object_cap   = OBJECT_CAP_DEFAULT,
    .camera       = {0},
};

struct GraphicsSys {
    Camera3D camera;
};
inline GraphicsSys graphics_sys = {};

void graphics_sys_init(GraphicsSysDesc desc = GRAPHICS_SYS_DESC_DEFAULT) {
    graphics_asset_sys_init(desc.texture_cap, desc.shader_cap, desc.material_cap, desc.mesh_cap);
    object_sys_init(desc.object_cap);
    graphics_sys.camera = desc.camera;

    // stub
    LIST_STUB(&graphics_asset_sys.texture_list); // TODO make it caro blackpink

    // defaults
    Image default_image = GenImageColor(1, 1, WHITE);
    Texture default_texture = LoadTextureFromImage(default_image);
    UnloadImage(default_image);
    list_create(&graphics_asset_sys.texture_list, &default_texture);
    graphics_asset_sys.texture_num++;

    Shader default_shader = LoadShader(0, 0);
    list_create(&graphics_asset_sys.shader_list, &default_shader);
    graphics_asset_sys.shader_num++;

    Material default_material = { .color = VEC4_ONE,
                                  .texture_key = TEXTURE_DEFAULT_KEY,
                                  .shader_key = SHADER_DEFAULT_KEY };
    list_create(&graphics_asset_sys.material_list, &default_material);
    graphics_asset_sys.material_num++;

    Mesh default_meshes[] = {
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

void graphics_update_frame() {
    object_update_frame();
}

void graphics_update_tick() {
    object_update_tick();
}

void graphics_draw() {
    BeginMode3D(graphics_sys.camera); {
        object_draw();
    } EndMode3D();
}

}
