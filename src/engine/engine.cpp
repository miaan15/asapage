module;

#include <raylib.h>

export module engine;

import define;

export import internal;
export import context;
export import log;
export import input;

export namespace asapage {

struct EngineConfigs {
    i32 tps;
};
constexpr EngineConfigs DEFAULT_ENGINE_CONFIGS = {
    .tps = 20
};

constexpr i32 DEFAULT_INIT_WINDOW_WIDTH = 1280;
constexpr i32 DEFAULT_INIT_WINDOW_HEIGHT = 720;
constexpr usize DEFAULT_OMNI_ARENA_SIZE = 700 << 20; // 700 MB
constexpr usize DEFAULT_TICK_ARENA_SIZE = 100 << 20; // 100 MB
constexpr usize DEFAULT_FRAME_ARENA_SIZE = 100 << 20; // 100 MB
struct EngineStaticConfigs {
    i32 init_window_width, init_window_height;
    const char *init_window_name;
    i64 omni_arena_size;
    i64 tick_arena_size;
    i64 frame_arena_size;
};

void engine_init(EngineConfigs *configs, EngineStaticConfigs *static_configs = nullptr) {
    i32 init_window_width = static_configs == nullptr ? DEFAULT_INIT_WINDOW_WIDTH : static_configs->init_window_width;
    i32 init_window_height = static_configs == nullptr ? DEFAULT_INIT_WINDOW_HEIGHT : static_configs->init_window_height;
    const char *init_window_name = static_configs == nullptr ? "" : static_configs->init_window_name;
    usize omni_arena_size = static_configs == nullptr ? DEFAULT_OMNI_ARENA_SIZE : static_configs->omni_arena_size;
    usize tick_arena_size = static_configs == nullptr ? DEFAULT_TICK_ARENA_SIZE : static_configs->tick_arena_size;
    usize frame_arena_size = static_configs == nullptr ? DEFAULT_FRAME_ARENA_SIZE : static_configs->frame_arena_size;

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

    // window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(init_window_width, init_window_height, init_window_name);
    SetExitKey(KEY_NULL);

    // screen
    screen_width = GetScreenWidth();
    screen_height = GetScreenHeight();

    // time
    time_realtime_last_frame = (f32)GetTime();
}

void engine_destroy() {
    CloseWindow();
    arena_destroy(&omni_arena);
}

bool engine_on_frame() {
    frame_arena_cur_idx = 1 - frame_arena_cur_idx;
    frame_arena_ptr = &frame_arena_buf[frame_arena_cur_idx];
    arena_reset(frame_arena_ptr);

    time_realtime_cur = GetTime();
    time_realtime_delta = time_realtime_cur - time_realtime_last_frame;
    time_realtime_last_frame = time_realtime_cur;
    tick_accumulated_time += time_realtime_delta;

    input_update_cache();

    if (IsWindowResized()) {
        screen_width = GetScreenWidth();
        screen_height = GetScreenHeight();
    }

    bool should_close = WindowShouldClose() || input_is_key_down(SCANCODE::ESCAPE);
    return !should_close;
}

bool engine_on_tick() {
    time_delta = 1 / (f32)TPS;
    bool is_on_tick = tick_accumulated_time > time_delta;
    if (is_on_tick) {
        tick_accumulated_time -= time_delta;

        tick_arena_cur_idx = 1 - tick_arena_cur_idx;
        tick_arena_ptr = &tick_arena_buf[tick_arena_cur_idx];
        arena_reset(tick_arena_ptr);
    }
    return is_on_tick;
}

void engine_draw() {
    interpolate_alpha = tick_accumulated_time / time_delta;

    BeginDrawing();
    ClearBackground(BLACK);

    //draw

    EndDrawing();
}

i32 engine_mem_offset(void *ptr) { return (char*)ptr - (char*)omni_arena_ptr->buf; }

}
