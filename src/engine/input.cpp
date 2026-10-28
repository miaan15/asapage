module;

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>

export module input;

import define;
import mem;
import internal;

export namespace asapage {

enum struct SCANCODE {
    UNKNOWN = SDL_SCANCODE_UNKNOWN,

    // letters
    A = SDL_SCANCODE_A,
    B = SDL_SCANCODE_B,
    C = SDL_SCANCODE_C,
    D = SDL_SCANCODE_D,
    E = SDL_SCANCODE_E,
    F = SDL_SCANCODE_F,
    G = SDL_SCANCODE_G,
    H = SDL_SCANCODE_H,
    I = SDL_SCANCODE_I,
    J = SDL_SCANCODE_J,
    K = SDL_SCANCODE_K,
    L = SDL_SCANCODE_L,
    M = SDL_SCANCODE_M,
    N = SDL_SCANCODE_N,
    O = SDL_SCANCODE_O,
    P = SDL_SCANCODE_P,
    Q = SDL_SCANCODE_Q,
    R = SDL_SCANCODE_R,
    S = SDL_SCANCODE_S,
    T = SDL_SCANCODE_T,
    U = SDL_SCANCODE_U,
    V = SDL_SCANCODE_V,
    W = SDL_SCANCODE_W,
    X = SDL_SCANCODE_X,
    Y = SDL_SCANCODE_Y,
    Z = SDL_SCANCODE_Z,

    // number row
    NUM_1 = SDL_SCANCODE_1,
    NUM_2 = SDL_SCANCODE_2,
    NUM_3 = SDL_SCANCODE_3,
    NUM_4 = SDL_SCANCODE_4,
    NUM_5 = SDL_SCANCODE_5,
    NUM_6 = SDL_SCANCODE_6,
    NUM_7 = SDL_SCANCODE_7,
    NUM_8 = SDL_SCANCODE_8,
    NUM_9 = SDL_SCANCODE_9,
    NUM_0 = SDL_SCANCODE_0,

    // control keys
    RETURN = SDL_SCANCODE_RETURN,
    ESCAPE = SDL_SCANCODE_ESCAPE,
    BACKSPACE = SDL_SCANCODE_BACKSPACE,
    TAB = SDL_SCANCODE_TAB,
    SPACE = SDL_SCANCODE_SPACE,
    CAPSLOCK = SDL_SCANCODE_CAPSLOCK,
    INSERT = SDL_SCANCODE_INSERT,
    HOME = SDL_SCANCODE_HOME,
    PAGEUP = SDL_SCANCODE_PAGEUP,
    DELETE = SDL_SCANCODE_DELETE,
    END = SDL_SCANCODE_END,
    PAGEDOWN = SDL_SCANCODE_PAGEDOWN,
    PRINTSCREEN = SDL_SCANCODE_PRINTSCREEN,
    SCROLLLOCK = SDL_SCANCODE_SCROLLLOCK,
    PAUSE = SDL_SCANCODE_PAUSE,

    // arrow keys
    RIGHT = SDL_SCANCODE_RIGHT,
    LEFT = SDL_SCANCODE_LEFT,
    DOWN = SDL_SCANCODE_DOWN,
    UP = SDL_SCANCODE_UP,

    // function keys
    F1 = SDL_SCANCODE_F1,
    F2 = SDL_SCANCODE_F2,
    F3 = SDL_SCANCODE_F3,
    F4 = SDL_SCANCODE_F4,
    F5 = SDL_SCANCODE_F5,
    F6 = SDL_SCANCODE_F6,
    F7 = SDL_SCANCODE_F7,
    F8 = SDL_SCANCODE_F8,
    F9 = SDL_SCANCODE_F9,
    F10 = SDL_SCANCODE_F10,
    F11 = SDL_SCANCODE_F11,
    F12 = SDL_SCANCODE_F12,

    // punctuation / symbols
    MINUS = SDL_SCANCODE_MINUS,
    EQUALS = SDL_SCANCODE_EQUALS,
    LEFTBRACKET = SDL_SCANCODE_LEFTBRACKET,
    RIGHTBRACKET = SDL_SCANCODE_RIGHTBRACKET,
    BACKSLASH = SDL_SCANCODE_BACKSLASH,
    SEMICOLON = SDL_SCANCODE_SEMICOLON,
    APOSTROPHE = SDL_SCANCODE_APOSTROPHE,
    GRAVE = SDL_SCANCODE_GRAVE,
    COMMA = SDL_SCANCODE_COMMA,
    PERIOD = SDL_SCANCODE_PERIOD,
    SLASH = SDL_SCANCODE_SLASH,

    // numpad
    NUMLOCKCLEAR = SDL_SCANCODE_NUMLOCKCLEAR,
    KP_DIVIDE = SDL_SCANCODE_KP_DIVIDE,
    KP_MULTIPLY = SDL_SCANCODE_KP_MULTIPLY,
    KP_MINUS = SDL_SCANCODE_KP_MINUS,
    KP_PLUS = SDL_SCANCODE_KP_PLUS,
    KP_ENTER = SDL_SCANCODE_KP_ENTER,
    KP_1 = SDL_SCANCODE_KP_1,
    KP_2 = SDL_SCANCODE_KP_2,
    KP_3 = SDL_SCANCODE_KP_3,
    KP_4 = SDL_SCANCODE_KP_4,
    KP_5 = SDL_SCANCODE_KP_5,
    KP_6 = SDL_SCANCODE_KP_6,
    KP_7 = SDL_SCANCODE_KP_7,
    KP_8 = SDL_SCANCODE_KP_8,
    KP_9 = SDL_SCANCODE_KP_9,
    KP_0 = SDL_SCANCODE_KP_0,
    KP_PERIOD = SDL_SCANCODE_KP_PERIOD,
    KP_EQUALS = SDL_SCANCODE_KP_EQUALS,

    // modifier keys
    LCTRL = SDL_SCANCODE_LCTRL,
    LSHIFT = SDL_SCANCODE_LSHIFT,
    LALT = SDL_SCANCODE_LALT,
    LGUI = SDL_SCANCODE_LGUI,
    RCTRL = SDL_SCANCODE_RCTRL,
    RSHIFT = SDL_SCANCODE_RSHIFT,
    RALT = SDL_SCANCODE_RALT,
    RGUI = SDL_SCANCODE_RGUI,
};

constexpr int INPUT_MAX_KEYBOARD_KEY_NUM = 512;

// the input has "cache" mechanism
// user's input can only be read in frame update, but usually input used for tick update (for gameplay/physics/...).
// "cache" is when the input between 2 tick preserved until the next tick happens
struct InputSys {
    const bool *cur_keyboard_state;
    bool *last_keyboard_state;
    u8 *cache_keyboard_state; // state as bitmask: 0-down, 1-on, 2-up
    int numkeys;
};
inline InputSys input_sys = {};

constexpr u8 _INPUT_CACHE_DOWN_STATE = 0b001;
constexpr u8 _INPUT_CACHE_ON_STATE = 0b010;
constexpr u8 _INPUT_CACHE_UP_STATE = 0b100;

void input_sys_init() {
    input_sys.cur_keyboard_state = SDL_GetKeyboardState(&input_sys.numkeys);
    input_sys.last_keyboard_state = (bool*)arena_alloc(&omni_arena, input_sys.numkeys);
    input_sys.cache_keyboard_state = (u8*)arena_alloc(&omni_arena, input_sys.numkeys);
}

bool input_is_key_down(SCANCODE scancode) {
    bool cur_state = input_sys.cur_keyboard_state[(usize)scancode];
    bool last_state = input_sys.last_keyboard_state[(usize)scancode];
    return cur_state && !last_state;
}

bool input_is_key_on(SCANCODE scancode) {
    bool cur_state = input_sys.cur_keyboard_state[(usize)scancode];
    return cur_state;
}

bool input_is_key_up(SCANCODE scancode) {
    bool cur_state = input_sys.cur_keyboard_state[(usize)scancode];
    bool last_state = input_sys.last_keyboard_state[(usize)scancode];
    return !cur_state && last_state;
}

bool input_is_key_down_cache(SCANCODE scancode) {
    u8 cache_state = input_sys.cache_keyboard_state[(usize)scancode];
    return (cache_state & _INPUT_CACHE_DOWN_STATE) != 0;
}

bool input_is_key_on_cache(SCANCODE scancode) {
    u8 cache_state = input_sys.cache_keyboard_state[(usize)scancode];
    bool is_cache_key_up = (cache_state & _INPUT_CACHE_UP_STATE) != 0;
    return ((cache_state & _INPUT_CACHE_ON_STATE) != 0) && !is_cache_key_up;
}

bool input_is_key_up_cache(SCANCODE scancode) {
    u8 cache_state = input_sys.cache_keyboard_state[(usize)scancode];
    bool is_cache_key_down = (cache_state & _INPUT_CACHE_DOWN_STATE) != 0;
    return ((cache_state & _INPUT_CACHE_UP_STATE) != 0) && !is_cache_key_down;
}

// call this before SDL_PollEvent
void input_update() {
    // update cache_keyboard_state
    for (i32 i = 0; i < input_sys.numkeys; i++) {
        bool cur_state = input_sys.cur_keyboard_state[i];
        bool last_state = input_sys.last_keyboard_state[i];

        bool is_down = cur_state && !last_state;
        bool is_up = !cur_state && last_state;
        bool is_on = cur_state;

        input_sys.cache_keyboard_state[i] |= (is_down ? _INPUT_CACHE_DOWN_STATE : 0) |
                                             (is_on   ? _INPUT_CACHE_ON_STATE   : 0) |
                                             (is_up   ? _INPUT_CACHE_UP_STATE   : 0);
    }

    // update last_keyboard_state
    mem_copy(input_sys.last_keyboard_state, input_sys.cur_keyboard_state, input_sys.numkeys * sizeof(bool));
}

// call this after all tick update input request
void input_reset_cache() {
    for (i32 i = 0; i < input_sys.numkeys; i++) {
        // if a key is "down" and "up" before a tick, delay the "up" state for 1 tick so that "down" and "up" not happens in single tick
        input_sys.cache_keyboard_state[i] =
            input_sys.cache_keyboard_state[i] == (_INPUT_CACHE_DOWN_STATE | _INPUT_CACHE_UP_STATE) ?
                _INPUT_CACHE_UP_STATE : 0;
    }
}

}
