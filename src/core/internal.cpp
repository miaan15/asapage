module;

#include <SDL3/SDL.h>

export module internal;

import define;
import mem;

export namespace asapage {

constexpr usize OMNI_ARENA_SIZE = 1000 << 10 << 10;
constexpr usize TICK_ARENA_SIZE = 50 << 10 << 10;
constexpr usize FRAME_ARENA_SIZE = 50 << 10 << 10;

// Arena
Arena omni_arena;

Arena tick_arena_buf[2];
usize tick_arena_cur_idx;

Arena frame_arena_buf[2];
usize frame_arena_cur_idx;

Arena *omni_arena_ptr;
Arena *tick_arena_ptr;
Arena *frame_arena_ptr;

// SDL
SDL_Window *sdl_window;
SDL_GPUDevice *gpu_device;

void internal_init() {
    // Arena
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
}

}
