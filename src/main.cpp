import define;
import mem;

import engine;

import file;

import math;
import pool;
import map;
import mapstr;

using namespace asapage;

int main() {
    engine_init();

    mesh_load(file_asset_path("monkey.glb").c_str());
    mesh_load(file_asset_path("box.glb").c_str());

    material_load((Vec4){1, 0, 0, 1});

    graphics_sys.draw_instance_list[graphics_sys.draw_instance_num++] = {
        .position = { 0, 0, 0 },
        .rotation = { 0, 0, 0, 1 },
        .scale = { 1, 1, 1 },
        .mesh_idx = 1,
        .material_idx = 1
    };

    graphics_sys.view_size = { (f32)screen_width, (f32)screen_height };
    graphics_sys.view_range = { 0.1, 100 };
    graphics_sys.view_eye = { 0, 15, 10 };
    graphics_sys.view_center = { 0, 0, 0 };
    graphics_sys.view_up = { 0, 1, 0 };
    graphics_sys.view_fov = 60 * MATH_DEG_TO_RAD;

    while (engine_on_frame()) {
        while (engine_on_tick()) {

        }

        engine_draw();
    }

    engine_destroy();
}
