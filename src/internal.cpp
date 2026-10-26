module;

#include "SDL3/SDL_gpu.h"
#include <SDL3/SDL.h>
#include <cstdio>

export module internal;

import define;
import mem;

export namespace asapage {

// config
constexpr i32 TPS = 20;
constexpr usize OMNI_ARENA_SIZE = 500 << 20; // 500 MB
constexpr usize TICK_ARENA_SIZE = 100 << 20; // 100 MB
constexpr usize FRAME_ARENA_SIZE = 100 << 20; // 100 MB

// arena
Arena omni_arena;

Arena tick_arena_buf[2];
usize tick_arena_cur_idx;

Arena frame_arena_buf[2];
usize frame_arena_cur_idx;

Arena *omni_arena_ptr;
Arena *tick_arena_ptr;
Arena *frame_arena_ptr;

// time
f32 time_cur, time_delta;

f32 tick_accumulated_time;

f32 time_realtime_cur, time_realtime_delta;
f32 time_realtime_last_frame;

f32 interpolate_alpha; // result by time

// SDL
SDL_Window *sdl_window;
SDL_GPUDevice *gpu_device;

// screen
i32 screen_width, screen_height;

void internal_init() {
    // arena
    arena_init(&omni_arena, OMNI_ARENA_SIZE);

    arena_init_over_arena(&tick_arena_buf[0], &omni_arena, TICK_ARENA_SIZE);
    arena_init_over_arena(&tick_arena_buf[1], &omni_arena, TICK_ARENA_SIZE);
    tick_arena_cur_idx = 0;

    arena_init_over_arena(&frame_arena_buf[0], &omni_arena, FRAME_ARENA_SIZE);
    arena_init_over_arena(&frame_arena_buf[1], &omni_arena, FRAME_ARENA_SIZE);
    frame_arena_cur_idx = 0;

    omni_arena_ptr = &omni_arena;
    tick_arena_ptr = &tick_arena_buf[tick_arena_cur_idx];
    frame_arena_ptr = &frame_arena_buf[frame_arena_cur_idx];

    // SDL
    SDL_Init(SDL_INIT_VIDEO);

    sdl_window = SDL_CreateWindow("", 1280, 720, SDL_WINDOW_RESIZABLE);

    gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);

    SDL_ClaimWindowForGPUDevice(gpu_device, sdl_window);

    // screen
    SDL_GetWindowSizeInPixels(sdl_window, &screen_width, &screen_height);
}

void internal_destroy() {
    arena_destroy(&omni_arena);
}

// updates
void internal_poll_events(bool *app_running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) { *app_running = false; }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) { *app_running = false; }

        if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            screen_width = event.window.data1;
            screen_height = event.window.data2;
        }
    }

}

void internal_early_frame_update() {
    frame_arena_cur_idx = 1 - frame_arena_cur_idx;
    frame_arena_ptr = &frame_arena_buf[frame_arena_cur_idx];
    arena_reset(frame_arena_ptr);

    time_realtime_cur = (f32)SDL_GetTicks() / 1000;
    time_realtime_delta = time_realtime_cur - time_realtime_last_frame;
    time_realtime_last_frame = time_realtime_cur;
    tick_accumulated_time += time_realtime_delta;
}

void internal_tick_update() {
    tick_arena_cur_idx = 1 - tick_arena_cur_idx;
    tick_arena_ptr = &tick_arena_buf[tick_arena_cur_idx];
    arena_reset(tick_arena_ptr);
}

void internal_pre_draw_update() {
    interpolate_alpha = tick_accumulated_time / time_delta;
}

// do 'while (internal_on_tick())'
bool internal_on_tick() {
    time_delta = 1 / (f32)TPS;
    bool res = tick_accumulated_time > time_delta;
    if (res) {
        tick_accumulated_time -= time_delta;
    }
    return res;
}

i32 internal_mem_offset(void *ptr) { return (char*)ptr - (char*)omni_arena_ptr->buf; }

}
