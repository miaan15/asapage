#include <raylib.h>

import define;
import mem;
import log;

import engine;
import graphics;

import file;
import math;
import pool;
import map;
import mapstr;

using namespace asapage;

MeshKey monkey_mesh_key, box_mesh_key;
ObjectKey monkey_object_key;
MaterialKey material_key;

Vec3 monkey_look_dir;
Vec3 monkey_desired_move_dir;

constexpr f32 MONKEY_MOVE_SPEED = 15;

int main() {
    engine_init();

    ModelLoadResult monkey_model_res = model_load(file_asset_path("monkey.glb").c_str());
    monkey_mesh_key = monkey_model_res.mesh_keys[0];

    ModelLoadResult box_model_res = model_load(file_asset_path("box.glb").c_str());
    box_mesh_key = box_model_res.mesh_keys[0];

    material_key = material_create({ 1, 0, 0, 1 });

    ObjectDesc monkey_object_desc = { .mesh_key = monkey_mesh_key, .material_key = material_key,
                                      .pos = VEC3_ZERO, .rot = QUAT_IDENTITY, .scale = VEC3_ONE };
    monkey_object_key = object_create(&monkey_object_desc);

    Camera3D *camera = &graphics_sys.camera;
    camera->position = { 0, 5, 8 };
    camera->target = { 0, 0, 0 };
    camera->up = { 0, 1, 0 };
    camera->fovy = 60;
    camera->projection = CAMERA_PERSPECTIVE;

    constexpr i32 TEST_WIDTH = 10;
    constexpr i32 TEST_HEIGHT = 10;
    constexpr f32 TEST_DISTANCE = 3;
    ObjectKey test_object_keys[TEST_WIDTH * TEST_HEIGHT];
    ObjectDesc test_object_desc = { .mesh_key = monkey_mesh_key, .material_key = material_key,
                                    .rot = QUAT_IDENTITY, .scale = VEC3_ONE };
    for (i32 i = 0; i < TEST_WIDTH; i++) {
        for (i32 j = 0; j < TEST_HEIGHT; j++) {
            test_object_desc.pos = { TEST_DISTANCE * (i - (TEST_WIDTH / 2)), 0, TEST_DISTANCE * (j - (TEST_HEIGHT / 2)) };
            test_object_keys[i + j * TEST_WIDTH] = object_create(&test_object_desc);
        }
    }

    while (engine_on_frame()) {
        while (engine_on_tick()) {
            // input
            monkey_desired_move_dir = VEC3_ZERO;
            if (IsKeyDown(KEY_W)) { monkey_desired_move_dir.z -= 1; }
            if (IsKeyDown(KEY_A)) { monkey_desired_move_dir.x -= 1; }
            if (IsKeyDown(KEY_S)) { monkey_desired_move_dir.z += 1; }
            if (IsKeyDown(KEY_D)) { monkey_desired_move_dir.x += 1; }
            monkey_desired_move_dir = vec3_normalize(monkey_desired_move_dir);

            // monke
            Object *monkey_object = object_get(monkey_object_key);
            monkey_object->pos += monkey_desired_move_dir * MONKEY_MOVE_SPEED * time_delta;

            for (i32 i = 0; i < TEST_WIDTH * TEST_HEIGHT; i++) {
                object_get(test_object_keys[i])->pos += monkey_desired_move_dir * MONKEY_MOVE_SPEED * time_delta;
            }
        }
        Vec3 monkey_pos = object_get(monkey_object_key)->pos;
        camera->target = { monkey_pos.x, monkey_pos.y, monkey_pos.z };

        engine_draw();
    }

    engine_destroy();
}
