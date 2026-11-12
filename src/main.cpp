import define;
import mem;

import engine;

import file;

import math;
import pool;
import map;
import mapstr;

using namespace asapage;

MeshKey monkey_mesh_key, box_mesh_key;
Object3DKey monkey_object_key;
MaterialKey material_key;

Vec3 monkey_look_dir;
Vec3 monkey_desired_move_dir;

constexpr f32 MONKEY_MOVE_SPEED = 15;

int main() {
    engine_init();

    mesh_load(file_asset_path("monkey.glb").c_str(), &monkey_mesh_key);
    mesh_load(file_asset_path("box.glb").c_str(), &box_mesh_key);

    MaterialDesc material_desc = { .albedo = { 1, 0, 0 } };
    material_key = material_create(&material_desc);

    Object3DDesc monkey_object_desc = { .mesh_key = monkey_mesh_key, .material_key = material_key,
                                        .pos = VEC3_ZERO, .rot = QUAT_IDENTITY, .scale = VEC3_ONE };
    monkey_object_key = object3d_create(&monkey_object_desc);

    FrameConstant *frame_constant = &graphics_sys.frame_constant;
    frame_constant->view_size = { (f32)screen_width, (f32)screen_height };
    frame_constant->view_range = { 0.1, 100 };
    frame_constant->view_eye = { 0, 5, 8 };
    frame_constant->view_center = { 0, 0, 0 };
    frame_constant->view_up = { 0, 1, 0 };
    frame_constant->view_fov = 60 * MATH_DEG_TO_RAD;

    constexpr i32 TEST_WIDTH = 50;
    constexpr i32 TEST_HEIGHT = 50;
    constexpr f32 TEST_DISTANE = 3;
    Object3DKey test_object_keys[TEST_WIDTH * TEST_HEIGHT];
    Object3DDesc test_object_desc = { .mesh_key = monkey_mesh_key, .material_key = material_key,
                                      .rot = QUAT_IDENTITY, .scale = VEC3_ONE };
    for (i32 i = 0; i < TEST_WIDTH; i++) {
        for (i32 j = 0; j < TEST_HEIGHT; j++) {
            test_object_desc.pos.x = TEST_DISTANE * (i - (TEST_WIDTH / 2));
            test_object_desc.pos.z = TEST_DISTANE * (j - (TEST_HEIGHT / 2));
            test_object_keys[i + j * TEST_WIDTH] = object3d_create(&test_object_desc);
        }
    }

    while (engine_on_frame()) {
        while (engine_on_tick()) {
            // input
            monkey_desired_move_dir = VEC3_ZERO;
            if (input_is_key_on_tick(SCANCODE::W)) { monkey_desired_move_dir.z -= 1; }
            if (input_is_key_on_tick(SCANCODE::A)) { monkey_desired_move_dir.x -= 1; }
            if (input_is_key_on_tick(SCANCODE::S)) { monkey_desired_move_dir.z += 1; }
            if (input_is_key_on_tick(SCANCODE::D)) { monkey_desired_move_dir.x += 1; }
            monkey_desired_move_dir = vec3_normalize(monkey_desired_move_dir);

            // monke
            Object3D *monkey_object = object3d_get(monkey_object_key);
            monkey_object->pos += monkey_desired_move_dir * MONKEY_MOVE_SPEED * time_delta;

            for (i32 i = 0; i < TEST_WIDTH * TEST_HEIGHT; i++) {
                object3d_get(test_object_keys[i])->pos += monkey_desired_move_dir * MONKEY_MOVE_SPEED * time_delta;
            }
        }
        frame_constant->view_center = object3d_get(monkey_object_key)->frame_pos;

        engine_draw();
    }

    engine_destroy();
}
