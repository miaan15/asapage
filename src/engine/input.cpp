module;

#include <raylib.h>

export module input;

import define;
import mem;
import internal;

export namespace asapage {

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

constexpr int INPUT_MAX_KEYBOARD_KEY_NUM = 512;

// the input has "cache" mechanism
// user's input can only be read in frame update, but usually input used for tick update (for gameplay/physics/...).
// "cache" is when the input between 2 tick preserved until the next tick happens
// # this was once built on SDL, but now raylib handle most of stuff, so this only has cache left
struct InputSys {
    u8 *cache_keyboard_state; // state as bitmask: 0-down, 1-on, 2-up
    int numkeys;
};
inline InputSys input_sys = {};

constexpr u8 _INPUT_CACHE_DOWN_STATE = 0b001;
constexpr u8 _INPUT_CACHE_ON_STATE = 0b010;
constexpr u8 _INPUT_CACHE_UP_STATE = 0b100;

void input_sys_init() {
    input_sys.numkeys = INPUT_MAX_KEYBOARD_KEY_NUM;
    input_sys.cache_keyboard_state = (u8*)arena_alloc(&omni_arena, input_sys.numkeys * sizeof(u8));
}

bool input_is_key_down(SCANCODE scancode) {
    return IsKeyPressed((i32)scancode);
}

bool input_is_key_on(SCANCODE scancode) {
    return IsKeyDown((i32)scancode);
}

bool input_is_key_up(SCANCODE scancode) {
    return IsKeyReleased((i32)scancode);
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

void input_update_cache() {
    for (i32 i = 0; i < input_sys.numkeys; i++) {
        bool is_down = IsKeyPressed(i);
        bool is_up = IsKeyDown(i);
        bool is_on = IsKeyReleased(i);
        input_sys.cache_keyboard_state[i] |= (is_down ? _INPUT_CACHE_DOWN_STATE : 0) |
                                             (is_on   ? _INPUT_CACHE_ON_STATE   : 0) |
                                             (is_up   ? _INPUT_CACHE_UP_STATE   : 0);
    }
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
