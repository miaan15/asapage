module;

#include <cstddef>
#include <raylib.h>

export module graphics_object3d;

import define;
import mem;
import internal;
import log;
import math;
import pool;
import list;

import graphics_model;
import graphics_material;

export namespace asapage {

struct Object3DKey {
    union {
        u64 raw;
        struct {
            i32 idx;
            i8 validate;
            u8 _reserve0;
            u16 _reserve1;
        };
    };
};

// TODO interpolate is quite too expensive rn, make it to GPU's work instead
struct Object3D {
    i32 idx;
    i8 validate;

    ModelKey model_key;
    MaterialKey material_key;

    Vec3 pos;
    Vec4 rot;
    Vec3 scale;

    Vec3 last_tick_pos;
    Vec4 last_tick_rot;

    Vec3 frame_pos; // interpolated pos, rot
    Vec4 frame_rot;
};

struct Object3DDesc {
    ModelKey model_key;
    MaterialKey material_key;

    Vec3 pos;
    Vec4 rot;
    Vec3 scale;
};

constexpr i32 OBJECT3D_CAP_DEFAULT = 1e5; // 100k

struct Object3DSys {
    Pool<Object3D> object_pool;
    i32 object_num;
};
inline Object3DSys object3d_sys = {};

void object3d_sys_init(i32 object_cap = OBJECT3D_CAP_DEFAULT) {
    pool_init(&object3d_sys.object_pool, omni_arena_ptr, object_cap,
              offsetof(Object3D, idx), offsetof(Object3D, validate));
    object3d_sys.object_num = 0;

    // TODO stub is all stub and pos = 0
}

Object3DKey object3d_create(Object3DDesc *desc = nullptr, Object3D *out_object = nullptr) {
    Object3D new_object_data;
    if (desc != nullptr) {
        new_object_data.model_key = desc->model_key;
        new_object_data.material_key = desc->material_key;

        new_object_data.pos = desc->pos;
        new_object_data.rot = desc->rot;
        new_object_data.scale = desc->scale;

        new_object_data.last_tick_pos = desc->pos;
        new_object_data.last_tick_rot = desc->rot;

        new_object_data.frame_pos = desc->pos;
        new_object_data.frame_rot = desc->rot;
    } else { mem_zero(&new_object_data, sizeof(Object3D)); }

    PoolKey pool_key = pool_create(&object3d_sys.object_pool, &new_object_data, &out_object);
    object3d_sys.object_num++;

    log_debug("Created Object3D [%d:%d]: model_key = [%d]; material_key = [%d], pos = [%.1f, %.1f, %.1f]; rot = [%.1f, %.1f, %.1f, %.1f]; scale = [%.1f, %.1f, %.1f]",
              pool_key.idx, pool_key.validate, desc->model_key.idx, desc->material_key.idx,
              desc->pos.x, desc->pos.y, desc->pos.z,
              desc->rot.x, desc->rot.y, desc->rot.z, desc->rot.w,
              desc->scale.x, desc->scale.y, desc->scale.z);

    return Object3DKey{ .idx = pool_key.idx, .validate = pool_key.validate };
}

bool object3d_destroy(Object3DKey key) {
    PoolKey pool_key = { key.idx, key.validate };
    bool res = pool_destroy(&object3d_sys.object_pool, pool_key);
    if (res) {
        object3d_sys.object_num--;
    }
    return res;
}

Object3D *object3d_get(Object3DKey key) {
    PoolKey pool_key = { .idx = key.idx, .validate = key.validate };
    return pool_get(&object3d_sys.object_pool, pool_key);
}

void object3d_update_frame() {
    i32 _pool_idx = 0;
    Object3D *iter_object;
    while (pool_iterate(&object3d_sys.object_pool, &_pool_idx, &iter_object, nullptr)) {
        iter_object->frame_pos = vec3_lerp(iter_object->last_tick_pos, iter_object->pos, interpolate_alpha);
        iter_object->frame_rot = quat_lerp(iter_object->last_tick_rot, iter_object->rot, interpolate_alpha);
    }
}

void object3d_update_tick() {
    i32 _pool_idx = 0;
    Object3D *iter_object;
    while (pool_iterate(&object3d_sys.object_pool, &_pool_idx, &iter_object, nullptr)) {
        iter_object->last_tick_pos = iter_object->pos;
        iter_object->last_tick_rot = iter_object->rot;
    }
}

void object3d_draw() {
    i32 _pool_idx = 0;
    Object3D *iter_object;
    while (pool_iterate(&object3d_sys.object_pool, &_pool_idx, &iter_object, nullptr)) {
        Model *model = model_get(iter_object->model_key);
        DrawModel(&model);
    }
}

}
