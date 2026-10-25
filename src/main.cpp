#include <cassert>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

import define;
import mem;
import internal;
import context;
import file;
import input;
import graphics;
import shader;

import math;
import pool;
import map;
import mapstr;

using namespace asapage;

int main() {
    internal_init();
    context_init();

    file_sys_init();

    input_sys_init();

    graphics_sys_init();

    graphics_sys.view = (GraphicsGPUView) {
        .size = { (f32)screen_width, (f32)screen_height },
        .range = { 0.1, 100 },
        .eye = { 0, 15, 10 },
        .center = { 0, 0, 0 },
        .up = { 0, 1, 0 },
        .fov = 60 * MATH_DEG_TO_RAD
    };

    graphics_sys.env = (GraphicsGPUEnv) {
        .light_pos = { 5, 10, 5 },
        .light_intensity = 1
    };

    i32 box_model_idx = graphics_load_model("box.glb");
    i32 monkey_model_idx = graphics_load_model("monkey.glb");

    GraphicsObject *object_box = graphics_create_object();
    object_box->model_idx = box_model_idx;
    object_box->color = { 1, 0, 0 };
    object_box->position = { -1.5, 0, 0 };
    object_box->rotation = { 0.71, 0, 0, 0.71 };
    object_box->scale = { 1, 2, 1 };

    GraphicsObject *object_monkey = graphics_create_object();
    object_monkey->model_idx = monkey_model_idx;
    object_monkey->color = { 0, 0, 1 };
    object_monkey->position = { 1.5, 0, 0 };
    object_monkey->rotation = { 0, 0, 0, 1 };
    object_monkey->scale = { 1, 1, 1 };

    constexpr f32 EYE_ROTATION_SPEED = 0.1;
    constexpr f32 LIGHT_ROTATION_SPEED = 0.1;
    f32 eye_rotation_y = 0;
    f32 eye_distance = 12;
    f32 light_rotation_x = 0;
    f32 light_distance = 100;

    bool app_running = true;
    while (app_running) {
        internal_frame_update();

        input_pre_poll();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { app_running = false; }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) { app_running = false; }
        }

        if (input_is_key_on(SCANCODE::A)) { eye_rotation_y += EYE_ROTATION_SPEED; }
        if (input_is_key_on(SCANCODE::D)) { eye_rotation_y -= EYE_ROTATION_SPEED; }
        if (input_is_key_on(SCANCODE::W)) { light_rotation_x += LIGHT_ROTATION_SPEED; }
        if (input_is_key_on(SCANCODE::S)) { light_rotation_x -= LIGHT_ROTATION_SPEED; }

        if (input_is_key_on(SCANCODE::UP)) { object_monkey->position.z -= .1; }
        if (input_is_key_on(SCANCODE::DOWN)) { object_monkey->position.z += .1; }
        if (input_is_key_on(SCANCODE::LEFT)) { object_monkey->position.x -= .1; }
        if (input_is_key_on(SCANCODE::RIGHT)) { object_monkey->position.x += .1; }

        graphics_sys.view.eye = mat4_rotation(quat_normalize((Quat) { 0, math_sin(eye_rotation_y / 2), 0, -math_cos(eye_rotation_y / 2) })) * VEC3_BACK * eye_distance;
        graphics_sys.view.eye.y += 10;

        graphics_sys.env.light_pos = mat4_rotation(quat_normalize((Quat) { math_sin(light_rotation_x / 2), 0, 0, -math_cos(light_rotation_x / 2) }))  * VEC3_BACK * light_distance;

        graphics_draw();
    }

    graphics_sys_destroy();

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}
