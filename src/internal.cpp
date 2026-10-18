module;

#include <SDL3/SDL.h>

export module internal;

import define;

export namespace asapage {

SDL_Window *window;
SDL_GPUDevice *gpu_device;

void internal_init() {
    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("", 800, 800, SDL_WINDOW_RESIZABLE);

    gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);

    SDL_ClaimWindowForGPUDevice(gpu_device, window);
}

}
