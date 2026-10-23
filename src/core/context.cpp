module;

#include <SDL3/SDL_video.h>

export module context;

import define;
import internal;

export namespace asapage {

i32 screen_width, screen_height;

void context_init() {
    SDL_GetWindowSizeInPixels(sdl_window, &screen_width, &screen_height);
}

void context_frame_update() {
    SDL_GetWindowSizeInPixels(sdl_window, &screen_width, &screen_height);
}

}
