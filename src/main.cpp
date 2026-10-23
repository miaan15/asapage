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

import graphics_model;

using namespace asapage;

int main() {
    internal_init();
    context_init();

    file_sys_init();

    input_sys_init();

    graphics_sys_init();

    graphics_sys.view = (GraphicsView) {
        .size = { (f32)screen_width, (f32)screen_height },
        .range = { 0.1, 100 },
        .eye = { 0, 2, 5 },
        .center = { 0, 0, 0 },
        .up = { 0, 1, 0 },
        .fov = 60 * MATH_DEG_TO_RAD
    };

    graphics_sys.env = (GraphicsEnv) {
        .light_pos = { 5, 10, 5 },
        .light_intensity = 1
    };

    i32 box_model_idx = graphics_load_model("box.glb");
    i32 monkey_model_idx = graphics_load_model("monkey.glb");

    GraphicsObject *object_box = graphics_create_object();
    object_box->model_idx = box_model_idx;
    object_box->color = { 1, 0, 0 };
    object_box->position = { -1.5, 0, 0 };
    object_box->rotation = { 0, 0, 0, 1 };
    object_box->scale = { 1, 1, 1 };

    GraphicsObject *object_monkey = graphics_create_object();
    object_monkey->model_idx = monkey_model_idx;
    object_monkey->color = { 0, 0, 1 };
    object_monkey->position = { 1.5, 0, 0 };
    object_monkey->rotation = { 0, 0, 0, 1 };
    object_monkey->scale = { 1, 1, 1 };

    bool app_running = true;
    while (app_running) {
        input_pre_poll();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { app_running = false; }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) { app_running = false; }
        }

        graphics_draw();
    }

    graphics_sys_destroy();

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}
