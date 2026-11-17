module;

#include <cassert>
#include <raylib.h>

export module graphics;

export import graphics_asset;
export import graphics_object;

import define;
import mem;
import internal;
import math;
import log;
import list;

export namespace asapage {

constexpr i32 MAX_LIGHT_NUM = 3;
struct Light {
    i32 enabled;
    Vec3 dir;
    Vec4 color;
};

struct GraphicsSysDesc {
    i32 texture_cap;
    i32 shader_cap;
    i32 material_cap;
    i32 mesh_cap;
    i32 object_cap;

    Camera3D camera;

    Vec4 ambient;
};

constexpr GraphicsSysDesc GRAPHICS_SYS_DESC_DEFAULT = {
    .texture_cap  = TEXTURE_CAP_DEFAULT,
    .shader_cap   = SHADER_CAP_DEFAULT,
    .material_cap = MATERIAL_CAP_DEFAULT,
    .mesh_cap     = MESH_CAP_DEFAULT,
    .object_cap   = OBJECT_CAP_DEFAULT,
    .camera       = {0},
    .ambient      = { 0, 0, 0, 1 },
};

struct GraphicsSys {
    Camera3D camera;
    Vec4 ambient;
    Light light_list[MAX_LIGHT_NUM];
};
inline GraphicsSys graphics_sys = {};

void graphics_sys_init(GraphicsSysDesc desc = GRAPHICS_SYS_DESC_DEFAULT) {
    graphics_asset_sys_init(desc.texture_cap, desc.shader_cap, desc.material_cap, desc.mesh_cap);
    object_sys_init(desc.object_cap);
    graphics_sys.camera = desc.camera;
    graphics_sys.ambient = desc.ambient;
}

void graphics_update_frame() {
    object_update_frame();
}

void graphics_update_tick() {
    object_update_tick();
}

void graphics_draw() {
    BeginMode3D(graphics_sys.camera); {
        constexpr usize MAX_RL_MATERIAL_MAPS = MATERIAL_MAP_BRDF + 1;
        ::MaterialMap maps[MAX_RL_MATERIAL_MAPS] = {};
        ::Material rl_material = { .maps = maps };

        i32 _pool_idx = 0;
        Object *iter_object;
        while (pool_iterate(&object_sys.object_pool, &_pool_idx, &iter_object, nullptr)) {
            Mesh mesh = iter_object->mesh;
            Material material = iter_object->material;
            Shader shader = material.shader;

            // FIXME later maybe I'll do some batching or cache
            shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

            int ambient_loc = GetShaderLocation(shader, "ambient");
            SetShaderValue(shader, ambient_loc, &graphics_sys.ambient, SHADER_UNIFORM_VEC4);

            for (i32 i = 0; i < MAX_LIGHT_NUM; i++) {
                int light_enabled_loc = GetShaderLocation(shader, TextFormat("lights[%i].enabled", i));
                int light_dir_loc     = GetShaderLocation(shader, TextFormat("lights[%i].dir", i));
                int light_color_loc   = GetShaderLocation(shader, TextFormat("lights[%i].color", i));

                SetShaderValue(shader, light_enabled_loc, &graphics_sys.light_list[i].enabled, SHADER_UNIFORM_INT);
                SetShaderValue(shader, light_dir_loc, &graphics_sys.light_list[i].dir, SHADER_UNIFORM_VEC3);
                SetShaderValue(shader, light_color_loc, &graphics_sys.light_list[i].color, SHADER_UNIFORM_VEC4);
            }

            rl_material.shader = material.shader;
            rl_material.maps[MATERIAL_MAP_DIFFUSE].texture = material.texture;
            rl_material.maps[MATERIAL_MAP_DIFFUSE].color = material.color;

            Mat4 transform = mat4_transpose(
                mat4_scale(iter_object->scale) *
                mat4_rotation(iter_object->frame_rot) *
                mat4_translation(iter_object->frame_pos)
            );
            Matrix rl_transform; mem_copy(&rl_transform, &transform, sizeof(Mat4));

            DrawMesh(mesh, rl_material, rl_transform);
        }
    } EndMode3D();
}

}
