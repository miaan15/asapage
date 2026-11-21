module;

#include <cassert>
#include <cfloat>
#include <raylib.h>
#include <raymath.h>

export module ui;

import define;
import mem;
import internal;
import log;
import math;
import list;
import pool;
import input;
import graphics;

export namespace asapage {

struct UISysDesc {
    i32 select_object_cap;
};

constexpr UISysDesc UI_SYS_DESC_DEFAULT = {
    .select_object_cap = OBJECT_CAP_DEFAULT
};

struct UISysConfig {
};

enum struct UI_SELECT_MODE { RESET, ADD, SUB };

struct UISys {
    UISysConfig config;

    List<ObjectKey> selected_object_key_list;

    UI_SELECT_MODE select_mode;
    bool is_selecting;
    bool is_dragbox;
    Vec2 select_dragbox_begin;
};
inline UISys ui_sys = {};

void ui_sys_init(UISysDesc desc = UI_SYS_DESC_DEFAULT) {
    list_init(&ui_sys.selected_object_key_list, omni_arena_ptr, desc.select_object_cap);
    ui_sys.select_mode = UI_SELECT_MODE::RESET;
}

bool _ui_raycast_object(Vec2 screen_pos, ObjectKey *out_key);
void _ui_dragbox_select_object(Vec2 begin_pos, Vec2 end_pos, ObjectKey **out_list_buf, i32 *out_list_len);

void ui_update_frame() {
    ui_sys.select_mode = UI_SELECT_MODE::RESET;
    if (input_is_key_on(SCANCODE::LSHIFT)) { ui_sys.select_mode = UI_SELECT_MODE::ADD; }
    if (input_is_key_on(SCANCODE::LALT))   { ui_sys.select_mode = UI_SELECT_MODE::SUB; }

    if (input_is_mouse_down(MOUSE_BUTTON::LEFT)) {
        ui_sys.is_selecting = true;
        ui_sys.is_dragbox = false;
        ui_sys.select_dragbox_begin = input_mouse_pos();
    }

    if (ui_sys.is_selecting && !ui_sys.is_dragbox) {
        constexpr f32 DRAGBOX_THRESHOLD = 6.0f;
        Vec2 mouse_pos = input_mouse_pos();
        f32 dx = mouse_pos.x - ui_sys.select_dragbox_begin.x;
        f32 dy = mouse_pos.y - ui_sys.select_dragbox_begin.y;
        if ((dx * dx + dy * dy) >= (DRAGBOX_THRESHOLD * DRAGBOX_THRESHOLD)) {
            ui_sys.is_dragbox = true;
        }
    }

    if (input_is_mouse_up(MOUSE_BUTTON::LEFT) && ui_sys.is_selecting) {
        Vec2 mouse_pos = input_mouse_pos();
        ObjectKey *selecting_list_buf = nullptr;
        i32 selecting_list_len = 0;

        if (ui_sys.is_dragbox) {
            _ui_dragbox_select_object(ui_sys.select_dragbox_begin, mouse_pos, &selecting_list_buf, &selecting_list_len);
        } else {
            ObjectKey hit_key = {};
            if (_ui_raycast_object(mouse_pos, &hit_key)) {
                selecting_list_buf = (ObjectKey*)arena_alloc(frame_arena_ptr, sizeof(ObjectKey));
                selecting_list_buf[0] = hit_key;
                selecting_list_len = 1;
            }
        }

        if (ui_sys.select_mode == UI_SELECT_MODE::RESET) {
            list_reset(&ui_sys.selected_object_key_list);
            assert(selecting_list_len <= ui_sys.selected_object_key_list.cap);
            if (selecting_list_len > 0) {
                mem_copy(LIST_BUF(&ui_sys.selected_object_key_list),
                         selecting_list_buf,
                         selecting_list_len * sizeof(ObjectKey));
                ui_sys.selected_object_key_list.len = selecting_list_len;
            }
        } else {
            for (i32 i = 0; i < selecting_list_len; i++) {
                ObjectKey selecting = selecting_list_buf[i];
                assert(object_is_alive(selecting));

                if (ui_sys.select_mode == UI_SELECT_MODE::ADD) {
                    bool already_selected = false;
                    i32 _idx = 0;
                    ObjectKey *iter_object_key = nullptr;
                    while (list_iterate(&ui_sys.selected_object_key_list, &_idx, &iter_object_key)) {
                        if (selecting == *iter_object_key) {
                            already_selected = true;
                            break;
                        }
                    }
                    if (!already_selected) {
                        assert(ui_sys.selected_object_key_list.len < ui_sys.selected_object_key_list.cap);
                        list_create(&ui_sys.selected_object_key_list, &selecting);
                    }
                } else if (ui_sys.select_mode == UI_SELECT_MODE::SUB) {
                    i32 _idx = 0;
                    ObjectKey *iter_object_key = nullptr;
                    i32 iter_idx = 0;
                    while (list_iterate(&ui_sys.selected_object_key_list, &_idx, &iter_object_key, &iter_idx)) {
                        if (selecting == *iter_object_key) {
                            list_destroy(&ui_sys.selected_object_key_list, iter_idx);
                            break;
                        }
                    }
                }
            }
        }

        ui_sys.is_selecting = false;
        ui_sys.is_dragbox = false;
    }
}

void ui_draw() {
    if (ui_sys.is_selecting && ui_sys.is_dragbox) {
        Vec2 mouse_pos = input_mouse_pos();
        Vec2 min_pos = {
            math_min(ui_sys.select_dragbox_begin.x, mouse_pos.x),
            math_min(ui_sys.select_dragbox_begin.y, mouse_pos.y)
        };
        Vec2 max_pos = {
            math_max(ui_sys.select_dragbox_begin.x, mouse_pos.x),
            math_max(ui_sys.select_dragbox_begin.y, mouse_pos.y)
        };

        Rectangle rect = {
            .x      = min_pos.x,
            .y      = min_pos.y,
            .width  = max_pos.x - min_pos.x,
            .height = max_pos.y - min_pos.y
        };

        DrawRectangleRec(rect, Color{ 80, 160, 255, 50 });
        DrawRectangleLinesEx(rect, 1.0f, Color{ 80, 160, 255, 220 });
    }

    // draw select
    i32 _idx = 0;
    ObjectKey *iter_object_key = nullptr;
    while (list_iterate(&ui_sys.selected_object_key_list, &_idx, &iter_object_key)) {
        Object *obj = object_get(*iter_object_key);
        if (obj == nullptr) { continue; }

        Vector3 world_pos = { obj->frame_pos.x, obj->frame_pos.y, obj->frame_pos.z };
        Vector2 screen_pos = GetWorldToScreen(world_pos, graphics_sys.camera);
        DrawCircleLines((i32)screen_pos.x, (i32)screen_pos.y, 12, RED);
    }
}

bool _ui_raycast_object(Vec2 screen_pos, ObjectKey *out_key) {
    assert(out_key != nullptr);

    Ray ray = GetScreenToWorldRay(Vector2{ screen_pos.x, screen_pos.y }, graphics_sys.camera);

    constexpr f32 SELECT_RADIUS = 1;
    f32 closest_dist = FLT_MAX;
    bool has_hit = false;

    // FIXME brute force
    i32 _pool_idx = 0;
    Object *iter_object = nullptr;
    PoolKey pool_key;
    while (pool_iterate(&object_sys.object_pool, &_pool_idx, &iter_object, &pool_key)) {
        Vector3 world_pos = { iter_object->frame_pos.x, iter_object->frame_pos.y, iter_object->frame_pos.z };
        RayCollision collision = GetRayCollisionSphere(ray, world_pos, SELECT_RADIUS);

        if (collision.hit && collision.distance < closest_dist) {
            closest_dist = collision.distance;
            *out_key = ObjectKey{ .idx = pool_key.idx, .validate = pool_key.validate };
            has_hit = true;
        }
    }

    return has_hit;
}

void _ui_dragbox_select_object(Vec2 begin_pos, Vec2 end_pos, ObjectKey **out_list_buf, i32 *out_list_len) {
    assert(out_list_buf != nullptr);
    assert(out_list_len != nullptr);

    constexpr f32 SELECT_PAD = 0;
    Vec2 min_pos = { math_min(begin_pos.x, end_pos.x) - SELECT_PAD, math_min(begin_pos.y, end_pos.y) - SELECT_PAD };
    Vec2 max_pos = { math_max(begin_pos.x, end_pos.x) + SELECT_PAD, math_max(begin_pos.y, end_pos.y) + SELECT_PAD };

    ObjectKey *list_buf = nullptr;
    i32 list_len = 0, list_cap = 0;

    Vector3 cam_forward = Vector3Normalize(Vector3Subtract(graphics_sys.camera.target, graphics_sys.camera.position));

    // FIXME brute force
    i32 _pool_idx = 0;
    Object *iter_object = nullptr;
    PoolKey pool_key;
    while (pool_iterate(&object_sys.object_pool, &_pool_idx, &iter_object, &pool_key)) {
        Vector3 world_pos = { iter_object->frame_pos.x, iter_object->frame_pos.y, iter_object->frame_pos.z };

        Vector3 to_obj = Vector3Subtract(world_pos, graphics_sys.camera.position);
        if (Vector3DotProduct(to_obj, cam_forward) <= 0) { continue; }

        Vector2 screen_pos = GetWorldToScreen(world_pos, graphics_sys.camera);
        if (screen_pos.x >= min_pos.x && screen_pos.x <= max_pos.x &&
            screen_pos.y >= min_pos.y && screen_pos.y <= max_pos.y) {
            if (list_len >= list_cap) {
                list_cap = list_cap == 0 ? 2 : list_cap * 2;
                ObjectKey *new_list_buf = (ObjectKey*)arena_alloc(frame_arena_ptr, list_cap * sizeof(ObjectKey));
                if (list_buf != nullptr) { mem_copy(new_list_buf, list_buf, list_len * sizeof(ObjectKey)); }
                list_buf = new_list_buf;
            }
            list_buf[list_len] = ObjectKey{ .idx = pool_key.idx, .validate = pool_key.validate };
            list_len++;
        }
    }

    *out_list_buf = list_buf;
    *out_list_len = list_len;
}

};
