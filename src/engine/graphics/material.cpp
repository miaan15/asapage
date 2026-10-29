module;

#include <glad/gl.h>

export module graphics_material;

import define;
import mem;
import internal;
import file;
import log;
import math;

export namespace asapage {

struct MaterialGPU {
    Vec4 base_color;
};

constexpr i32 MATERIAL_MAX_MATERIAL = 1024;
constexpr usize MATERIAL_MAX_MATERIAL_BUF_SIZE = 100 << 20; // 100MB

struct MaterialSys {
    GLuint material_buf;

    i32 material_num;
    i32 material_cap;
};
inline MaterialSys material_sys = {};

void material_sys_init(i32 material_cap = MATERIAL_MAX_MATERIAL,
                       usize max_material_buf_size = MATERIAL_MAX_MATERIAL_BUF_SIZE) {
    glGenBuffers(1, &material_sys.material_buf);
    glBindBuffer(GL_ARRAY_BUFFER, material_sys.material_buf);
    glBufferData(GL_ARRAY_BUFFER, max_material_buf_size, nullptr, GL_DYNAMIC_DRAW);

    material_sys.material_cap = material_cap;

    // stub
    material_sys.material_num = 1;
    // TODO make stub material like the caro black-pink
}

void material_load(Vec4 base_color = (Vec4){ 1, 1, 1, 1 }) { // might be more complex with many textures, but this is for now
    if (material_sys.material_num >= material_sys.material_cap) {
        log_err("material_load: too many material: num = [%d]", material_sys.material_num);
        return;
    }

    i32 material_idx = material_sys.material_num;

    MaterialGPU _material = {
        .base_color = base_color
    };

    usize meterial_buf_offset = material_idx * sizeof(MaterialGPU);
    glBindBuffer(GL_ARRAY_BUFFER, material_sys.material_buf);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        meterial_buf_offset,
        sizeof(MaterialGPU),
        &_material
    );
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    material_sys.material_num++;

    log_debug("Load material [%d]: base_color = [%.1f %.1f %.1f %.1f]",
              material_idx, base_color.x, base_color.y, base_color.z, base_color.w);
}

}
