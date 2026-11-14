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
