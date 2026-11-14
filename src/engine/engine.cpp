module;

#include <glad.h>
#include <raylib.h>

export module engine;

import define;
import mem;

export import internal;
export import context;
export import log;
export import file;
export import input;
export import graphics;

export namespace asapage {

struct EngineConfigs {
    i32 tps;
};
inline EngineConfigs engine_configs = {
    .tps = 20
};

constexpr i32 WINDOW_WIDTH_DEFAULT = 1280;
constexpr i32 WINDOW_HEIGHT_DEFAULT = 720;
constexpr usize OMNI_ARENA_SIZE_DEFAULT = 700 << 20; // 700 MB
constexpr usize TICK_ARENA_SIZE_DEFAULT = 100 << 20; // 100 MB
constexpr usize FRAME_ARENA_SIZE_DEFAULT = 100 << 20; // 100 MB

struct EngineDesc {
    i32 window_width, window_height;
    const char *window_name;
    i64 omni_arena_size;
    i64 tick_arena_size;
    i64 frame_arena_size;
};

constexpr EngineDesc ENGINE_DESC_DEFAULT = {
    .window_width     = WINDOW_WIDTH_DEFAULT,
    .window_height    = WINDOW_HEIGHT_DEFAULT,
    .window_name      = "",
    .omni_arena_size  = OMNI_ARENA_SIZE_DEFAULT,
    .tick_arena_size  = TICK_ARENA_SIZE_DEFAULT,
    .frame_arena_size = FRAME_ARENA_SIZE_DEFAULT,
};

void engine_init(EngineDesc desc = ENGINE_DESC_DEFAULT) {
    i32 window_width        = desc.window_width;
    i32 window_height       = desc.window_height;
    const char *window_name = desc.window_name != nullptr ? desc.window_name : "";
    usize omni_arena_size   = desc.omni_arena_size;
    usize tick_arena_size   = desc.tick_arena_size;
    usize frame_arena_size  = desc.frame_arena_size;

    // raylib
    SetConfigFlags(FLAG_WINDOW_UNDECORATED);
    InitWindow(window_width, window_height, window_name);

    // arena
    arena_init(&omni_arena, omni_arena_size);

    arena_init_over_arena(&tick_arena_buf[0], &omni_arena, tick_arena_size);
    arena_init_over_arena(&tick_arena_buf[1], &omni_arena, tick_arena_size);
    tick_arena_cur_idx = 0;

    arena_init_over_arena(&frame_arena_buf[0], &omni_arena, frame_arena_size);
    arena_init_over_arena(&frame_arena_buf[1], &omni_arena, frame_arena_size);
    frame_arena_cur_idx = 0;

    omni_arena_ptr = &omni_arena;
    tick_arena_ptr = &tick_arena_buf[tick_arena_cur_idx];
    frame_arena_ptr = &frame_arena_buf[frame_arena_cur_idx];

    // screen
    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();

    // time
    time_realtime_last_frame = (f32)GetTime();

    // sys
    file_sys_init();

    input_sys_init();

    graphics_sys_init();
}

void engine_destroy() {
    CloseWindow();
    arena_destroy(&omni_arena);
}

bool engine_on_frame() {
    // arena swap
    frame_arena_cur_idx = 1 - frame_arena_cur_idx;
    frame_arena_ptr = &frame_arena_buf[frame_arena_cur_idx];
    arena_reset(frame_arena_ptr);

    // update time
    time_realtime_cur = GetTime();
    time_realtime_delta = time_realtime_cur - time_realtime_last_frame;
    time_realtime_last_frame = time_realtime_cur;
    tick_accumulated_time += time_realtime_delta;

    // cache input for tick
    input_update_frame();

    // screen
    if (IsWindowResized()) {
        screen_width = GetScreenWidth();
        screen_height = GetScreenHeight();
    }

    //
    bool should_close = WindowShouldClose() || input_is_key_down(SCANCODE::ESCAPE);
    return !should_close;
}

bool engine_on_tick() {
    time_delta = 1 / (f32)engine_configs.tps;
    bool is_on_tick = tick_accumulated_time > time_delta;
    if (is_on_tick) {
        tick_accumulated_time -= time_delta;

        // arena swap
        tick_arena_cur_idx = 1 - tick_arena_cur_idx;
        tick_arena_ptr = &tick_arena_buf[tick_arena_cur_idx];
        arena_reset(tick_arena_ptr);

        //
        input_update_tick();
        graphics_update_tick();
    }
    return is_on_tick;
}

void engine_draw() {
    // interpolate
    interpolate_alpha = tick_accumulated_time / time_delta;

    graphics_update_frame();

    BeginDrawing();
    ClearBackground(BLACK);

    graphics_draw();

    DrawFPS(10, 10);

    EndDrawing();
}

i32 engine_mem_offset(void *ptr) { return (char*)ptr - (char*)omni_arena_ptr->buf; }

}
