module;

#include <SDL3/SDL.h>

export module internal;

import define;
import mem;
import graphics_core;

export namespace asapage {

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

f32 interpolate_alpha;

// SDL
SDL_Window *sdl_window;
SDL_GLContext gl_context;

// screen
i32 screen_width, screen_height;

}
