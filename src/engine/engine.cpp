module;

#include <SDL3/SDL.h>
#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>

export module engine;

import define;

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

EngineConfigs engine_configs = {
    .tps = 20
};

constexpr i32 DEFAULT_INIT_WINDOW_WIDTH = 1280;
constexpr i32 DEFAULT_INIT_WINDOW_HEIGHT = 720;
constexpr usize DEFAULT_OMNI_ARENA_SIZE = 700 << 20; // 700 MB
constexpr usize DEFAULT_TICK_ARENA_SIZE = 100 << 20; // 100 MB
constexpr usize DEFAULT_FRAME_ARENA_SIZE = 100 << 20; // 100 MB
struct EngineInitConfigs {
    i32 init_window_width, init_window_height;
    const char *init_window_name;
    i64 omni_arena_size;
    i64 tick_arena_size;
    i64 frame_arena_size;
};

void engine_init(EngineInitConfigs *static_configs = nullptr) {
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

    // SDL
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    sdl_window = SDL_CreateWindow(
        init_window_name,
        init_window_width,
        init_window_height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL
    );

    gl_context = SDL_GL_CreateContext(sdl_window);
    SDL_GL_MakeCurrent(sdl_window, gl_context);

    gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);

    SDL_GetWindowSizeInPixels(sdl_window, &screen_width, &screen_height);

    // time
    time_realtime_last_frame = (f32)SDL_GetTicks() / 1000;

    file_sys_init();

    shader_sys_init();
    mesh_sys_init();
    material_sys_init();
    graphics_sys_init();
}

void engine_destroy() {
    SDL_DestroyWindow(sdl_window);
    arena_destroy(&omni_arena);
}

bool engine_on_frame() {
    // arena swap
    frame_arena_cur_idx = 1 - frame_arena_cur_idx;
    frame_arena_ptr = &frame_arena_buf[frame_arena_cur_idx];
    arena_reset(frame_arena_ptr);

    // update time
    time_realtime_cur = (f32)SDL_GetTicks() / 1000;
    time_realtime_delta = time_realtime_cur - time_realtime_last_frame;
    time_realtime_last_frame = time_realtime_cur;
    tick_accumulated_time += time_realtime_delta;

    // cache input for tick
    input_update();

    bool is_on_frame = true;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) { is_on_frame = false; }
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) {
            is_on_frame = false;
        }

        if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            screen_width = event.window.data1;
            screen_height = event.window.data2;
        }
    }

    return is_on_frame;
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
    }
    return is_on_tick;
}

void engine_draw() {
    // interpolate
    interpolate_alpha = tick_accumulated_time / time_delta;

    glViewport(0, 0, screen_width, screen_height);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    graphics_draw();

    SDL_GL_SwapWindow(sdl_window);
}

i32 engine_mem_offset(void *ptr) { return (char*)ptr - (char*)omni_arena_ptr->buf; }

}
