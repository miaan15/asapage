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

    MeshKey monkey_mesh_key, box_mesh_key;
    mesh_load(file_asset_path("monkey.glb").c_str(), &monkey_mesh_key);
    mesh_load(file_asset_path("box.glb").c_str(), &box_mesh_key);

    MaterialDesc material_desc = { .albedo = { 1, 0, 0 } };
    MaterialKey material_key = material_create(&material_desc);

    Object3DDesc object_desc = { .material_key = material_key, .rot = QUAT_IDENTITY, .scale = VEC3_ONE };
    object_desc.mesh_key = monkey_mesh_key; {
        object_desc.pos = { 0, 0, 0 };
        object3d_create(&object_desc);

        object_desc.pos = { 4, 0, 0 };
        object3d_create(&object_desc);

        object_desc.pos = { -4, 0, 0 };
        object3d_create(&object_desc);
    }
    object_desc.mesh_key = box_mesh_key; {
        object_desc.pos = { 0, -2, 0 };
        object3d_create(&object_desc);

        object_desc.pos = { 3, -2, 0 };
        object3d_create(&object_desc);

        object_desc.pos = { -3, -2, 0 };
        object3d_create(&object_desc);
    }

    FrameConstant *frame_constant = &graphics_sys.frame_constant;
    frame_constant->view_size = { (f32)screen_width, (f32)screen_height };
    frame_constant->view_range = { 0.1, 100 };
    frame_constant->view_eye = { 0, 5, 8 };
    frame_constant->view_center = { 0, 0, 0 };
    frame_constant->view_up = { 0, 1, 0 };
    frame_constant->view_fov = 60 * MATH_DEG_TO_RAD;

    while (engine_on_frame()) {
        while (engine_on_tick()) {

        }

        engine_draw();
    }

    engine_destroy();
}
