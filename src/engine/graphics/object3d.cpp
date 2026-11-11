module;

#include <cstddef>
#include <glad.h>

export module graphics_object3d;

import define;
import mem;
import internal;
import log;
import math;
import pool;
import list;

import graphics_mesh;
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

struct Object3D {
    i32 idx;
    i16 validate;

    MeshKey mesh_key;
    MaterialKey material_key;

    Vec3 pos;
    Vec4 rot;
    Vec3 scale;
};

struct Object3DDesc {
    MeshKey mesh_key;
    MaterialKey material_key;

    Vec3 pos;
    Vec4 rot;
    Vec3 scale;
};

struct Object3DGPUMap {
    i32 mesh_vertex_offset;
    i32 mesh_index_offset;

    i32 material_idx;

    f32 _pad0;

    Mat4 model_matrix; // maybe should not matrix from the beginning, matrix calculatation should be on GPU
};

constexpr i32 OBJECT3D_CAP_DEFAULT = 1e5; // 100k

struct Object3DSys {
    GLuint object_gpu_list;

    Pool<Object3D> object_pool;

    List<Object3DGPUMap> draw_object_gpu_map_list;
    List<i32> draw_object_indices_num_list;
    i32 draw_object_num;
};
inline Object3DSys object3d_sys = {};

void object3d_sys_init(i32 object_cap = OBJECT3D_CAP_DEFAULT) {
    glGenBuffers(1, &object3d_sys.object_gpu_list);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, object3d_sys.object_gpu_list);
    glBufferData(GL_SHADER_STORAGE_BUFFER, object_cap * sizeof(Object3DGPUMap), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    pool_init(&object3d_sys.object_pool, omni_arena_ptr, object_cap,
              offsetof(Object3D, idx), offsetof(Object3D, validate));

    list_init(&object3d_sys.draw_object_gpu_map_list, omni_arena_ptr, object_cap);
    list_init(&object3d_sys.draw_object_indices_num_list, omni_arena_ptr, object_cap);
    object3d_sys.draw_object_num = 0;

    // TODO stub is all stub and pos = 0
}

Object3DKey object3d_stub() { return Object3DKey{0}; }

Object3DKey object3d_create(Object3DDesc *desc = nullptr, Object3D *out_object = nullptr) {
    Object3D new_object_data;
    if (desc != nullptr) {
        new_object_data.pos = desc->pos;
        new_object_data.rot = desc->rot;
        new_object_data.scale = desc->scale;
        new_object_data.mesh_key = desc->mesh_key;
        new_object_data.material_key = desc->material_key;
    } else { mem_zero(&new_object_data, sizeof(Object3D)); }

    PoolKey pool_key = pool_create(&object3d_sys.object_pool, &new_object_data, &out_object);

    log_debug("Created Object3D [%d:%d]: mesh_key = [%d]; material_key = [%d], pos = [%.1f, %.1f, %.1f]; rot = [%.1f, %.1f, %.1f, %.1f]; scale = [%.1f, %.1f, %.1f]",
              pool_key.idx, pool_key.validate, desc->mesh_key.idx, desc->material_key.idx,
              desc->pos.x, desc->pos.y, desc->pos.z,
              desc->rot.x, desc->rot.y, desc->rot.z, desc->rot.w,
              desc->scale.x, desc->scale.y, desc->scale.z);

    return Object3DKey{ .idx = pool_key.idx, .validate = pool_key.validate };
}

bool object3d_destroy(Object3DKey key) {
    PoolKey pool_key = { key.idx, key.validate };
    return pool_destroy(&object3d_sys.object_pool, pool_key);
}

void object3d_upload_to_gpu() {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, object3d_sys.object_gpu_list); {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                        0, object3d_sys.draw_object_gpu_map_list.len * sizeof(Object3DGPUMap),
                        object3d_sys.draw_object_gpu_map_list.buf);
    } glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void object3d_update() {
    list_reset(&object3d_sys.draw_object_gpu_map_list);
    list_reset(&object3d_sys.draw_object_indices_num_list);
    object3d_sys.draw_object_num = 0;

    i32 _pool_idx = 0;
    Object3D *iter_object;
    while (pool_iterate(&object3d_sys.object_pool, &_pool_idx, &iter_object, nullptr)) {
        MeshMeta object_mesh_meta = mesh_get_meta(iter_object->mesh_key);

        Object3DGPUMap object_gpu_map = {
            .mesh_vertex_offset = object_mesh_meta.vertex_offset,
            .mesh_index_offset = object_mesh_meta.index_offset,

            .material_idx = iter_object->material_key.idx,

            .model_matrix = mat4_translation(iter_object->pos) *
                            mat4_rotation(iter_object->rot) *
                            mat4_scale(iter_object->scale)
        };

        list_create(&object3d_sys.draw_object_gpu_map_list, &object_gpu_map);
        list_create(&object3d_sys.draw_object_indices_num_list, &object_mesh_meta.indices_num);
        object3d_sys.draw_object_num++;
    }
}

}
