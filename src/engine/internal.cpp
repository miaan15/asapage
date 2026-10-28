module;

#include <raylib.h>

export module internal;

import define;
import mem;
import graphics_core;

export namespace asapage {

// config
constexpr i32 TPS = 20;
constexpr usize OMNI_ARENA_SIZE = 500 << 20; // 500 MB
constexpr usize TICK_ARENA_SIZE = 100 << 20; // 100 MB
constexpr usize FRAME_ARENA_SIZE = 100 << 20; // 100 MB
constexpr i32 SCREEN_WIDTH = 1280, SCREEN_HEIGHT = 720;

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

// screen
i32 screen_width, screen_height;

//
// void internal_init() {
//     // arena
//     arena_init(&omni_arena, OMNI_ARENA_SIZE);
//
//     arena_init_over_arena(&tick_arena_buf[0], &omni_arena, TICK_ARENA_SIZE);
//     arena_init_over_arena(&tick_arena_buf[1], &omni_arena, TICK_ARENA_SIZE);
//     tick_arena_cur_idx = 0;
//
//     arena_init_over_arena(&frame_arena_buf[0], &omni_arena, FRAME_ARENA_SIZE);
//     arena_init_over_arena(&frame_arena_buf[1], &omni_arena, FRAME_ARENA_SIZE);
//     frame_arena_cur_idx = 0;
//
//     omni_arena_ptr = &omni_arena;
//     tick_arena_ptr = &tick_arena_buf[tick_arena_cur_idx];
//     frame_arena_ptr = &frame_arena_buf[frame_arena_cur_idx];
//
//     // window
//     SetConfigFlags(FLAG_WINDOW_RESIZABLE);
//     InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "");
//
//     // screen
//     window_width = GetScreenWidth();
//     screen_height = GetScreenHeight();
//
//     // time
//     time_realtime_last_frame = (f32)GetTime();
// }
//
// void internal_destroy() {
//     CloseWindow();
//     arena_destroy(&omni_arena);
// }
//
// // updates
// void internal_poll_events(bool *app_running) {
//     if (WindowShouldClose() || IsKeyPressed(KEY_ESCAPE)) {
//         *app_running = false;
//     }
//
//     if (IsWindowResized()) {
//         window_width = GetScreenWidth();
//         screen_height = GetScreenHeight();
//     }
// }
//
// void internal_early_frame_update() {
//     frame_arena_cur_idx = 1 - frame_arena_cur_idx;
//     frame_arena_ptr = &frame_arena_buf[frame_arena_cur_idx];
//     arena_reset(frame_arena_ptr);
//
//     time_realtime_cur = GetTime();
//     time_realtime_delta = time_realtime_cur - time_realtime_last_frame;
//     time_realtime_last_frame = time_realtime_cur;
//     tick_accumulated_time += time_realtime_delta;
// }
//
// void internal_tick_update() {
//     tick_arena_cur_idx = 1 - tick_arena_cur_idx;
//     tick_arena_ptr = &tick_arena_buf[tick_arena_cur_idx];
//     arena_reset(tick_arena_ptr);
// }
//
// void internal_pre_draw_update() {
//     interpolate_alpha = tick_accumulated_time / time_delta;
// }
//
// // do 'while (internal_on_tick())'
// bool internal_on_tick() {
//     time_delta = 1 / (f32)TPS;
//     bool res = tick_accumulated_time > time_delta;
//     if (res) {
//         tick_accumulated_time -= time_delta;
//     }
//     return res;
// }
//
// i32 internal_mem_offset(void *ptr) { return (char*)ptr - (char*)omni_arena_ptr->buf; }

}
