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

Vec3 monkey_look_dir;
Vec3 monkey_desired_move_dir;

Camera3D *camera;

Vec3 camera_desired_move_dir;
Vec3 camera_pos;
f32 camera_yaw, camera_pitch;

constexpr f32 MONKEY_MOVE_SPEED = 15;

constexpr f32 CAMERA_MOVE_SPEED = 10;
constexpr f32 CAMERA_SENSIVITY = 0.005;

void handle_camera_control();

int main() {
    EngineDesc engine_desc = ENGINE_DESC_DEFAULT;
    engine_desc.window_width = 1600;
    engine_desc.window_height = 900;
    engine_init(engine_desc);

    ModelLoadResult monkey_model_res = model_load(file_asset_path("monkey.glb").c_str());
    MeshKey monkey_mesh_key = monkey_model_res.mesh_keys[0];

    ModelLoadResult box_model_res = model_load(file_asset_path("box.glb").c_str());
    MeshKey box_mesh_key = box_model_res.mesh_keys[0];

    ObjectDesc monkey_object_desc = { .mesh_key = monkey_mesh_key, .material_key = MATERIAL_UNLIT_DEFAULT_KEY,
                                      .pos = VEC3_ZERO, .rot = QUAT_IDENTITY, .scale = VEC3_ONE };
    ObjectKey monkey_object_key = object_create(&monkey_object_desc);

    camera = &graphics_sys.camera;
    // camera->position = { 0, 5, 8 };
    // camera->target = { 0, 0, 0 };
    camera->up = { 0, 1, 0 };
    camera->fovy = 60;
    camera->projection = CAMERA_PERSPECTIVE;
    //
    camera_pos = {0, 0, 8};

    constexpr i32 TEST_WIDTH = 0;
    constexpr i32 TEST_HEIGHT = 10;
    constexpr f32 TEST_DISTANCE = 3;
    ObjectKey test_object_keys[TEST_WIDTH * TEST_HEIGHT];
    ObjectDesc test_object_desc = { .mesh_key = monkey_mesh_key, .material_key = MATERIAL_UNLIT_DEFAULT_KEY,
                                    .rot = QUAT_IDENTITY, .scale = VEC3_ONE };
    for (i32 i = 0; i < TEST_WIDTH; i++) {
        for (i32 j = 0; j < TEST_HEIGHT; j++) {
            test_object_desc.pos = { TEST_DISTANCE * (i - i32(TEST_WIDTH / 2)), 0, TEST_DISTANCE * (j - i32(TEST_HEIGHT / 2)) };
            test_object_keys[i + j * TEST_WIDTH] = object_create(&test_object_desc);
        }
    }

    while (engine_on_frame()) {
        while (engine_on_tick()) {
            // input
            monkey_desired_move_dir = VEC3_ZERO;
            if (input_is_key_on_tick(SCANCODE::UP)) { monkey_desired_move_dir.z -= 1; }
            if (input_is_key_on_tick(SCANCODE::LEFT)) { monkey_desired_move_dir.x -= 1; }
            if (input_is_key_on_tick(SCANCODE::RIGHT)) { monkey_desired_move_dir.z += 1; }
            if (input_is_key_on_tick(SCANCODE::DOWN)) { monkey_desired_move_dir.x += 1; }
            monkey_desired_move_dir = vec3_normalize(monkey_desired_move_dir);

            // monke
            Object *monkey_object = object_get(monkey_object_key);
            monkey_object->pos += monkey_desired_move_dir * MONKEY_MOVE_SPEED * time_delta;

            for (i32 i = 0; i < TEST_WIDTH * TEST_HEIGHT; i++) {
                object_get(test_object_keys[i])->pos += monkey_desired_move_dir * MONKEY_MOVE_SPEED * time_delta;
            }
        }

        handle_camera_control();

        engine_draw();
    }

    engine_destroy();
}

void handle_camera_control() {
    if (input_is_mouse_on(MOUSE_BUTTON::RIGHT)) {
        Vec2 delta = input_mouse_delta();
        camera_yaw += delta.x * CAMERA_SENSIVITY;
        camera_pitch -= delta.y * CAMERA_SENSIVITY;

        if (camera_pitch > MATH_PI / 2 - 0.01) camera_pitch = MATH_PI / 2 - 0.01;
        if (camera_pitch < -MATH_PI / 2 + 0.01) camera_pitch = -MATH_PI / 2 + 0.01;
    }

    Vec3 forward = {
        math_sin(camera_yaw) * math_cos(camera_pitch),
        math_sin(camera_pitch),
        -math_cos(camera_yaw) * math_cos(camera_pitch)
    };
    forward = vec3_normalize(forward);
    Vec3 right = vec3_normalize(vec3_cross(forward, { 0, 1, 0 }));

    camera_desired_move_dir = VEC3_ZERO;
    if (input_is_key_on(SCANCODE::W)) { camera_desired_move_dir += forward; }
    if (input_is_key_on(SCANCODE::S)) { camera_desired_move_dir -= forward; }
    if (input_is_key_on(SCANCODE::D)) { camera_desired_move_dir += right; }
    if (input_is_key_on(SCANCODE::A)) { camera_desired_move_dir -= right; }
    if (input_is_key_on(SCANCODE::E)) { camera_desired_move_dir.y += 1; }
    if (input_is_key_on(SCANCODE::Q)) { camera_desired_move_dir.y -= 1; }
    camera_desired_move_dir = vec3_normalize(camera_desired_move_dir);

    camera_pos += camera_desired_move_dir * CAMERA_MOVE_SPEED * time_realtime_delta;

    Vec3 target = camera_pos + forward;
    mem_copy(&camera->position, &camera_pos, sizeof(Vec3));
    mem_copy(&camera->target, &target, sizeof(Vec3));
}
