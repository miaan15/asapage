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

    Material material;

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
    Vec3 pos;
    Vec4 rot;
    Vec3 scale;
};

struct ObjectSys {
    Pool<Object> object_pool;
    i32 object_num;
};
inline ObjectSys object_sys = {};

void object_sys_init(i32 object_cap) {
    pool_init(&object_sys.object_pool, omni_arena_ptr, object_cap,
              offsetof(Object, idx), offsetof(Object, validate));
    object_sys.object_num = 0;

    // TODO stub is all stub and pos = 0
}

ObjectKey object_create(ObjectDesc *desc = nullptr, Object **out_object = nullptr) {
    Object *new_object;
    PoolKey pool_key = pool_create(&object_sys.object_pool, (Object*)nullptr, &new_object);
    new_object->mesh_key = desc->mesh_key;
    new_object->material = *material_get(desc->material_key);
    new_object->pos = new_object->last_tick_pos = new_object->frame_pos = desc->pos;
    new_object->rot = new_object->last_tick_rot = new_object->frame_rot = desc->rot;
    new_object->scale = desc->scale;

    object_sys.object_num++;

    if (out_object != nullptr) { *out_object = new_object; }

    log_debug("Created Object [%d:%d]: mesh_key = [%d]; material_key = [%d]; pos = [%.1f, %.1f, %.1f]; rot = [%.1f, %.1f, %.1f, %.1f]; scale = [%.1f, %.1f, %.1f]",
              pool_key.idx, pool_key.validate,
              new_object->mesh_key.idx, desc->material_key.idx,
              new_object->pos.x, new_object->pos.y, new_object->pos.z,
              new_object->rot.x, new_object->rot.y, new_object->rot.z, new_object->rot.w,
              new_object->scale.x, new_object->scale.y, new_object->scale.z);

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
    RlMaterial rl_material = { .maps = maps };

    i32 _pool_idx = 0;
    Object *iter_object;
    while (pool_iterate(&object_sys.object_pool, &_pool_idx, &iter_object, nullptr)) {
        RlMesh *mesh = mesh_get(iter_object->mesh_key);
        Material *material = &iter_object->material;
        RlShader *shader = shader_get(material->shader_key);

        rl_material.shader = *shader;

        if (material->shader_key.idx == SHADER_UNLIT_KEY.idx) {
            ShaderUnlitData *data = (ShaderUnlitData *)material->data;
            RlTexture *texture = texture_get(data->texture_key);

            rl_material.maps[MATERIAL_MAP_DIFFUSE].texture = *texture;
            rl_material.maps[MATERIAL_MAP_DIFFUSE].color = Color{
                (u8)(data->color.x * 255.0f),
                (u8)(data->color.y * 255.0f),
                (u8)(data->color.z * 255.0f),
                255
            };
        } else if (material->shader_key.idx == SHADER_LIT_KEY.idx) {
            ShaderLitData *data = (ShaderLitData *)material->data;
            RlTexture *texture = texture_get(data->texture_key);

            rl_material.maps[MATERIAL_MAP_ALBEDO].texture = *texture;
            rl_material.maps[MATERIAL_MAP_ALBEDO].color = Color{
                (u8)(data->albedo.x * 255.0f),
                (u8)(data->albedo.y * 255.0f),
                (u8)(data->albedo.z * 255.0f),
                255
            };
            rl_material.maps[MATERIAL_MAP_METALNESS].value = data->metallic;
            rl_material.maps[MATERIAL_MAP_ROUGHNESS].value = 1.0f - data->smoothness;
        } else {
            rl_material.maps[MATERIAL_MAP_DIFFUSE].texture = *texture_get(TEXTURE_DEFAULT_KEY);
            rl_material.maps[MATERIAL_MAP_DIFFUSE].color = WHITE;
        }

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
