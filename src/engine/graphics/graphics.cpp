module;

#include <raylib.h>

export module graphics;

export import graphics_shader;
export import graphics_object3d;
export import graphics_model;
export import graphics_material;
export import graphics_asset;

import define;
import mem;
import internal;
import math;
import log;
import list;

export namespace asapage {

struct GraphicsSysDesc {
    i32 model_cap;
    i32 material_cap;
    i32 object_cap;

    Camera camera;
};

struct GraphicsSys {
    Camera camera;
};
inline GraphicsSys graphics_sys = {};

void graphics_sys_init(GraphicsSysDesc *desc = nullptr) {
    if (desc != nullptr) {
        model_sys_init(desc->model_cap);
        material_sys_init(desc->material_cap);
        object3d_sys_init(desc->object_cap);
        graphics_sys.camera = desc->camera;
    } else {
        model_sys_init();
        material_sys_init();
        object3d_sys_init();
        graphics_sys.camera = {0};
    }
}

void graphics_update_frame() {
    object3d_update_frame();
}

void graphics_update_tick() {
    object3d_update_tick();
}

void graphics_draw() {
    BeginMode3D(graphics_sys.camera); {
        object3d_draw();
    } EndMode3D();
}

}
