module;

#include <raylib.h>

export module input;

import define;
import mem;
import internal;
import log;
import math;

export namespace asapage {

enum struct SCANCODE;
enum struct MOUSE_BUTTON;

constexpr int INPUT_MAX_KEYBOARD_KEY_NUM = 512;
constexpr int INPUT_MAX_MOUSE_BUTTON_NUM = 8;

// the input has "cache" mechanism
// user's input can only be read in frame update, but usually input used for tick update (for gameplay/physics/...).
// "cache" is when the input between 2 tick preserved until the next tick happens
// # this was once built on SDL, but now raylib handle most of stuff, so this only has cache left
struct InputSys {
    u8 *cache_keyboard_state; // state as bitmask: 0-down, 1-on, 2-up
    u8 *tick_keyboard_state;
    int keyboard_key_num;

    u8 *cache_mouse_state;
    u8 *tick_mouse_state;
    int mouse_button_num;
};
inline InputSys input_sys = {};

constexpr u8 _INPUT_CACHE_DOWN_STATE = 0b001;
constexpr u8 _INPUT_CACHE_ON_STATE = 0b010;
constexpr u8 _INPUT_CACHE_UP_STATE = 0b100;

void input_sys_init() {
    input_sys.keyboard_key_num = INPUT_MAX_KEYBOARD_KEY_NUM;
    input_sys.cache_keyboard_state = (u8*)arena_alloc(&omni_arena, input_sys.keyboard_key_num * sizeof(u8));
    input_sys.tick_keyboard_state = (u8*)arena_alloc(&omni_arena, input_sys.keyboard_key_num * sizeof(u8));

    input_sys.mouse_button_num = INPUT_MAX_MOUSE_BUTTON_NUM;
    input_sys.cache_mouse_state = (u8*)arena_alloc(&omni_arena, input_sys.mouse_button_num * sizeof(u8));
    input_sys.tick_mouse_state  = (u8*)arena_alloc(&omni_arena, input_sys.mouse_button_num * sizeof(u8));
}

// helper
bool _input_has_bit(u8 x, u8 m) { return (x & m) != 0; } // ONLY USE FOR 1-BIT BITMASK

bool _input_cache_is_down(u8 state) { return _input_has_bit(state, _INPUT_CACHE_DOWN_STATE); }
bool _input_cache_is_on(u8 state) { return _input_has_bit(state, _INPUT_CACHE_ON_STATE); }
bool _input_cache_is_up(u8 state) { bool is_on = _input_has_bit(state, _INPUT_CACHE_ON_STATE);
    bool is_up = _input_has_bit(state, _INPUT_CACHE_UP_STATE);
    return is_up && !is_on;
}

void _input_sync_tick_buffer(u8 *cache, u8 *tick, i32 count) {
    for (i32 i = 0; i < count; i++) {
        u8 state = cache[i];
        tick[i] = (_input_cache_is_down(state) ? _INPUT_CACHE_DOWN_STATE : 0) |
                  (_input_cache_is_on(state)   ? _INPUT_CACHE_ON_STATE   : 0) |
                  (_input_cache_is_up(state)   ? _INPUT_CACHE_UP_STATE   : 0);

        // if a button/key is "down" and "up" before a tick, delay the "up" state for 1 tick
        cache[i] = (_input_has_bit(state, _INPUT_CACHE_ON_STATE) &&
                    _input_has_bit(state, _INPUT_CACHE_UP_STATE))
                       ? _INPUT_CACHE_UP_STATE
                       : 0;
    }
}

// keyboard
bool input_is_key_down(SCANCODE scancode) {
    return IsKeyPressed((i32)scancode);
}

bool input_is_key_on(SCANCODE scancode) {
    return IsKeyDown((i32)scancode);
}

bool input_is_key_up(SCANCODE scancode) {
    return IsKeyReleased((i32)scancode);
}

bool input_is_key_down_tick(SCANCODE scancode) {
    return _input_has_bit(input_sys.tick_keyboard_state[(i32)scancode], _INPUT_CACHE_DOWN_STATE);
}

bool input_is_key_on_tick(SCANCODE scancode) {
    return _input_has_bit(input_sys.tick_keyboard_state[(i32)scancode], _INPUT_CACHE_ON_STATE);
}

bool input_is_key_up_tick(SCANCODE scancode) {
    return _input_has_bit(input_sys.tick_keyboard_state[(i32)scancode], _INPUT_CACHE_UP_STATE);
}

// mouse
bool input_is_mouse_down(MOUSE_BUTTON button) {
    return IsMouseButtonPressed((i32)button);
}

bool input_is_mouse_on(MOUSE_BUTTON button) {
    return IsMouseButtonDown((i32)button);
}

bool input_is_mouse_up(MOUSE_BUTTON button) {
    return IsMouseButtonReleased((i32)button);
}

Vec2 input_mouse_position() {
    Vec2 res; Vector2 res_rl = GetMousePosition();
    mem_copy(&res, &res_rl, sizeof(Vec2));
    return res;
}

Vec2 input_mouse_delta() {
    Vec2 res; Vector2 res_rl = GetMouseDelta();
    mem_copy(&res, &res_rl, sizeof(Vec2));
    return res;
}

f32 input_mouse_wheel() {
    return GetMouseWheelMove();
}

//
void input_update_frame() {
    // keyboard
    for (i32 i = 0; i < input_sys.keyboard_key_num; i++) {
        bool is_down = input_is_key_down((SCANCODE)i);
        bool is_on   = input_is_key_on((SCANCODE)i);
        bool is_up   = input_is_key_up((SCANCODE)i);
        input_sys.cache_keyboard_state[i] |= (is_down ? _INPUT_CACHE_DOWN_STATE : 0) |
                                             (is_on   ? _INPUT_CACHE_ON_STATE   : 0) |
                                             (is_up   ? _INPUT_CACHE_UP_STATE   : 0);
    }

    // mouse
    for (i32 i = 0; i < input_sys.mouse_button_num; i++) {
        bool is_down = input_is_mouse_down((MOUSE_BUTTON)i);
        bool is_on   = input_is_mouse_on((MOUSE_BUTTON)i);
        bool is_up   = input_is_mouse_up((MOUSE_BUTTON)i);
        input_sys.cache_mouse_state[i] |= (is_down ? _INPUT_CACHE_DOWN_STATE : 0) |
                                          (is_on   ? _INPUT_CACHE_ON_STATE   : 0) |
                                          (is_up   ? _INPUT_CACHE_UP_STATE   : 0);
    }
}

// call this after all tick update input request
void input_update_tick() {
    _input_sync_tick_buffer(input_sys.cache_keyboard_state, input_sys.tick_keyboard_state, input_sys.keyboard_key_num);
    _input_sync_tick_buffer(input_sys.cache_mouse_state, input_sys.tick_mouse_state, input_sys.mouse_button_num);
}

bool _input_cache_is_key_down(usize idx) {
    u8 cache_state = input_sys.cache_keyboard_state[idx];
    bool is_cache_key_down = _input_has_bit(cache_state, _INPUT_CACHE_DOWN_STATE);
    return is_cache_key_down;
}

bool _input_cache_is_key_on(usize idx) {
    u8 cache_state = input_sys.cache_keyboard_state[idx];
    bool is_cache_key_on = _input_has_bit(cache_state, _INPUT_CACHE_ON_STATE);
    return is_cache_key_on;
}

bool _input_cache_is_key_up(usize idx) {
    u8 cache_state = input_sys.cache_keyboard_state[idx];
    bool is_cache_key_on = _input_has_bit(cache_state, _INPUT_CACHE_ON_STATE);
    bool is_cache_key_up = _input_has_bit(cache_state, _INPUT_CACHE_UP_STATE);
    return is_cache_key_up && !is_cache_key_on;
}

enum struct SCANCODE {
    NIL = KEY_NULL,

    // letters
    A = KEY_A,
    B = KEY_B,
    C = KEY_C,
    D = KEY_D,
    E = KEY_E,
    F = KEY_F,
    G = KEY_G,
    H = KEY_H,
    I = KEY_I,
    J = KEY_J,
    K = KEY_K,
    L = KEY_L,
    M = KEY_M,
    N = KEY_N,
    O = KEY_O,
    P = KEY_P,
    Q = KEY_Q,
    R = KEY_R,
    S = KEY_S,
    T = KEY_T,
    U = KEY_U,
    V = KEY_V,
    W = KEY_W,
    X = KEY_X,
    Y = KEY_Y,
    Z = KEY_Z,

    // number row
    NUM_1 = KEY_ONE,
    NUM_2 = KEY_TWO,
    NUM_3 = KEY_THREE,
    NUM_4 = KEY_FOUR,
    NUM_5 = KEY_FIVE,
    NUM_6 = KEY_SIX,
    NUM_7 = KEY_SEVEN,
    NUM_8 = KEY_EIGHT,
    NUM_9 = KEY_NINE,
    NUM_0 = KEY_ZERO,

    // control keys
    RETURN = KEY_ENTER,
    ESCAPE = KEY_ESCAPE,
    BACKSPACE = KEY_BACKSPACE,
    TAB = KEY_TAB,
    SPACE = KEY_SPACE,
    CAPSLOCK = KEY_CAPS_LOCK,
    INSERT = KEY_INSERT,
    HOME = KEY_HOME,
    PAGEUP = KEY_PAGE_UP,
    DELETE = KEY_DELETE,
    END = KEY_END,
    PAGEDOWN = KEY_PAGE_DOWN,
    PRINTSCREEN = KEY_PRINT_SCREEN,
    SCROLLLOCK = KEY_SCROLL_LOCK,
    PAUSE = KEY_PAUSE,

    // arrow keys
    RIGHT = KEY_RIGHT,
    LEFT = KEY_LEFT,
    DOWN = KEY_DOWN,
    UP = KEY_UP,

    // function keys
    F1 = KEY_F1,
    F2 = KEY_F2,
    F3 = KEY_F3,
    F4 = KEY_F4,
    F5 = KEY_F5,
    F6 = KEY_F6,
    F7 = KEY_F7,
    F8 = KEY_F8,
    F9 = KEY_F9,
    F10 = KEY_F10,
    F11 = KEY_F11,
    F12 = KEY_F12,

    // punctuation / symbols
    MINUS = KEY_MINUS,
    EQUALS = KEY_EQUAL,
    LEFTBRACKET = KEY_LEFT_BRACKET,
    RIGHTBRACKET = KEY_RIGHT_BRACKET,
    BACKSLASH = KEY_BACKSLASH,
    SEMICOLON = KEY_SEMICOLON,
    APOSTROPHE = KEY_APOSTROPHE,
    GRAVE = KEY_GRAVE,
    COMMA = KEY_COMMA,
    PERIOD = KEY_PERIOD,
    SLASH = KEY_SLASH,

    // numpad
    NUMLOCKCLEAR = KEY_NUM_LOCK,
    KP_DIVIDE = KEY_KP_DIVIDE,
    KP_MULTIPLY = KEY_KP_MULTIPLY,
    KP_MINUS = KEY_KP_SUBTRACT,
    KP_PLUS = KEY_KP_ADD,
    KP_ENTER = KEY_KP_ENTER,
    KP_1 = KEY_KP_1,
    KP_2 = KEY_KP_2,
    KP_3 = KEY_KP_3,
    KP_4 = KEY_KP_4,
    KP_5 = KEY_KP_5,
    KP_6 = KEY_KP_6,
    KP_7 = KEY_KP_7,
    KP_8 = KEY_KP_8,
    KP_9 = KEY_KP_9,
    KP_0 = KEY_KP_0,
    KP_PERIOD = KEY_KP_DECIMAL,
    KP_EQUALS = KEY_KP_EQUAL,

    // modifier keys
    LCTRL = KEY_LEFT_CONTROL,
    LSHIFT = KEY_LEFT_SHIFT,
    LALT = KEY_LEFT_ALT,
    LGUI = KEY_LEFT_SUPER,
    RCTRL = KEY_RIGHT_CONTROL,
    RSHIFT = KEY_RIGHT_SHIFT,
    RALT = KEY_RIGHT_ALT,
    RGUI = KEY_RIGHT_SUPER,
};

enum struct MOUSE_BUTTON {
    LEFT    = MOUSE_BUTTON_LEFT,
    RIGHT   = MOUSE_BUTTON_RIGHT,
    MIDDLE  = MOUSE_BUTTON_MIDDLE,
    SIDE    = MOUSE_BUTTON_SIDE,
    EXTRA   = MOUSE_BUTTON_EXTRA,
    FORWARD = MOUSE_BUTTON_FORWARD,
    BACK    = MOUSE_BUTTON_BACK,
};

}
