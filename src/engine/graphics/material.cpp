module;

#include <glad.h>

export module graphics_material;

import define;
import mem;
import internal;
import file;
import log;
import math;
import list;

export namespace asapage {

struct MaterialKey {
    union {
        u64 raw;
        struct {
            i32 idx;
            u32 _reserve;
        };
    };
};

struct Material {
    Vec3 albedo;
};

using MaterialDesc = Material;

struct alignas(16) MaterialGPUMap {
    Vec3 albedo;
};

constexpr i32 MATERIAL_CAP_DEFAULT = 1024;

struct MaterialSys {
    GLuint material_gpu_list;

    List<Material> material_list;
    List<MaterialGPUMap> material_gpu_map_list;
};
inline MaterialSys material_sys = {};

void material_sys_init(i32 material_cap = MATERIAL_CAP_DEFAULT) {
    glGenBuffers(1, &material_sys.material_gpu_list);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, material_sys.material_gpu_list);
    glBufferData(GL_SHADER_STORAGE_BUFFER, material_cap * sizeof(MaterialGPUMap), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    list_init(&material_sys.material_list, omni_arena_ptr, material_cap);
    list_init(&material_sys.material_gpu_map_list, omni_arena_ptr, material_cap);

    // TODO add stub, maybe the no shadow full color thingy
}

MaterialKey material_stub() { return MaterialKey{0}; }

MaterialKey material_create(MaterialDesc *desc = nullptr, Material *out_material = nullptr) {
    Material new_material_data;
    if (desc != nullptr) { mem_copy(&new_material_data, desc, sizeof(Material)); }
    else { mem_zero(&new_material_data, sizeof(Material)); }

    i32 idx = list_create(&material_sys.material_list, &new_material_data, &out_material);

    log_debug("Created Material [%d]: albeno = [%.1f, %.1f, %.1f]", desc->albedo.x, desc->albedo.y, desc->albedo.z);

    return (MaterialKey){.idx = idx};
}

Material *material_get(MaterialKey key) {
    if (key.idx <= 0 || key.idx >= material_sys.material_list.len) {
        log_err("material_get(): try to get with key has out of bounds idx -> stub: key = [%d]", key.idx);
        return list_stub(&material_sys.material_list);
    }
    return list_get(&material_sys.material_list, key.idx);
}

void material_upload_to_gpu() {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, material_sys.material_gpu_list); {
        glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                        0, material_sys.material_gpu_map_list.len * sizeof(MaterialGPUMap),
                        &material_sys.material_gpu_map_list);
    } glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void material_update() {
    for (i32 i = 1; i < material_sys.material_list.len; i++) {
        Material *material = list_get(&material_sys.material_list, i);
        MaterialGPUMap *material_gpu_map = list_get(&material_sys.material_gpu_map_list, i);

        material_gpu_map->albedo = material->albedo;
    }
}

}
