module;

#include <cstddef>
#include <raylib.h>

export module graphics_object;

import define;
import mem;
import internal;
import log;
import math;
import pool;
import list;

import graphics_asset;

export namespace asapage {

struct ObjectKey {
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

// TODO interpolate is quite expensive, make it to GPU's work instead
struct Object {
    i32 idx;
    i8 validate;

    MeshKey mesh_key;

    MaterialKey material_key;
    Vec4 color;
    TextureKey texture_key;
    ShaderKey shader_key;

    Vec3 pos;
    Vec4 rot;
    Vec3 scale;

    Vec3 last_tick_pos;
    Vec4 last_tick_rot;

    Vec3 frame_pos; // interpolated pos, rot
    Vec4 frame_rot;
};

struct ObjectDesc {
    MeshKey mesh_key;

    MaterialKey material_key;

    Vec4 color;             bool has_color;
    TextureKey texture_key; bool has_texture;
    ShaderKey shader_key;   bool has_shader;

    Vec3 pos;
    Vec4 rot;
    Vec3 scale;
};

constexpr i32 OBJECT_CAP_DEFAULT = 1e5; // 100k

struct ObjectSys {
    Pool<Object> object_pool;
    i32 object_num;
};
inline ObjectSys object_sys = {};

void object_sys_init(i32 object_cap = OBJECT_CAP_DEFAULT) {
    pool_init(&object_sys.object_pool, omni_arena_ptr, object_cap,
              offsetof(Object, idx), offsetof(Object, validate));
    object_sys.object_num = 0;

    // TODO stub is all stub and pos = 0
}

ObjectKey object_create(ObjectDesc *desc = nullptr, Object *out_object = nullptr) {
    Object new_object;
    if (desc != nullptr) {
        // FIXME this should handle default and stub value instead of just assign right like this
        new_object.mesh_key = desc->mesh_key;

        Material *material = material_get(desc->material_key);
        new_object.material_key = desc->material_key;
        new_object.color = material->color;
        new_object.texture_key = material->texture_key;
        new_object.shader_key = material->shader_key;

        if (desc->has_color) { new_object.color = desc->color; }
        if (desc->has_texture) { new_object.texture_key = desc->texture_key; }
        if (desc->has_shader) { new_object.shader_key = desc->shader_key; }

        new_object.pos = new_object.last_tick_pos = new_object.frame_pos = desc->pos;
        new_object.rot = new_object.last_tick_rot = new_object.frame_rot = desc->rot;
        new_object.scale = desc->scale;
    } else { mem_zero(&new_object, sizeof(Object)); }

    PoolKey pool_key = pool_create(&object_sys.object_pool, &new_object, &out_object);
    object_sys.object_num++;

    log_debug("Created Object [%d:%d]: mesh_key = [%d]; material_key = [%d]; color = [%.1f, %.1f, %.1f, %.1f]; texture = [%d]; shader = [%d]; pos = [%.1f, %.1f, %.1f]; rot = [%.1f, %.1f, %.1f, %.1f]; scale = [%.1f, %.1f, %.1f]",
              pool_key.idx, pool_key.validate,
              new_object.mesh_key.idx, new_object.material_key.idx,
              new_object.color.r, new_object.color.g, new_object.color.b, new_object.color.a,
              new_object.texture_key.idx,
              new_object.shader_key.idx,
              new_object.pos.x, new_object.pos.y, new_object.pos.z,
              new_object.rot.x, new_object.rot.y, new_object.rot.z, new_object.rot.w,
              new_object.scale.x, new_object.scale.y, new_object.scale.z);

    return ObjectKey{ .idx = pool_key.idx, .validate = pool_key.validate };
}

bool object_destroy(ObjectKey key) {
    PoolKey pool_key = { key.idx, key.validate };
    bool res = pool_destroy(&object_sys.object_pool, pool_key);
    if (res) {
        object_sys.object_num--;
    }
    return res;
}

Object *object_get(ObjectKey key) {
    PoolKey pool_key = { .idx = key.idx, .validate = key.validate };
    return pool_get(&object_sys.object_pool, pool_key);
}

void object_update_frame() {
    i32 _pool_idx = 0;
    Object *iter_object;
    while (pool_iterate(&object_sys.object_pool, &_pool_idx, &iter_object, nullptr)) {
        iter_object->frame_pos = vec3_lerp(iter_object->last_tick_pos, iter_object->pos, interpolate_alpha);
        iter_object->frame_rot = quat_lerp(iter_object->last_tick_rot, iter_object->rot, interpolate_alpha);
    }
}

void object_update_tick() {
    i32 _pool_idx = 0;
    Object *iter_object;
    while (pool_iterate(&object_sys.object_pool, &_pool_idx, &iter_object, nullptr)) {
        iter_object->last_tick_pos = iter_object->pos;
        iter_object->last_tick_rot = iter_object->rot;
    }
}

void object_draw() {
    constexpr usize MAX_RL_MATERIAL_MAPS = MATERIAL_MAP_BRDF + 1;
    ::MaterialMap maps[MAX_RL_MATERIAL_MAPS] = {};
    ::Material rl_material = { .maps = maps };

    i32 _pool_idx = 0;
    Object *iter_object;
    while (pool_iterate(&object_sys.object_pool, &_pool_idx, &iter_object, nullptr)) {
        Mesh *mesh = mesh_get(iter_object->mesh_key);
        Vec4 color = iter_object->color;
        Texture *texture = texture_get(iter_object->texture_key);
        Shader *shader = shader_get(iter_object->shader_key);

        rl_material.shader = *shader;
        rl_material.maps[MATERIAL_MAP_DIFFUSE].texture = *texture;
        rl_material.maps[MATERIAL_MAP_DIFFUSE].color = Color{
            (u8)(color.x * 255.0f),
            (u8)(color.y * 255.0f),
            (u8)(color.z * 255.0f),
            (u8)(color.w * 255.0f)
        };

        Mat4 transform = mat4_transpose(
            mat4_scale(iter_object->scale) *
            mat4_rotation(iter_object->frame_rot) *
            mat4_translation(iter_object->frame_pos)
        );
        Matrix rl_transform; mem_copy(&rl_transform, &transform, sizeof(Mat4));

        DrawMesh(*mesh, rl_material, rl_transform);
    }
}

}
