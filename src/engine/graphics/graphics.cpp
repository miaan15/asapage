module;

#include <cstdlib>
#include <glad.h>

export module graphics;

export import graphics_shader;
export import graphics_mesh;
export import graphics_material;

import define;
import mem;
import internal;
import math;
import log;

export namespace asapage {

struct alignas(16) FrameConstantGPU {
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Vec3 eye_pos;
    f32 _pad0;
};

struct alignas(16) InstanceDataGPU {
    i32 mesh_vertex_offset;
    i32 mesh_index_offset;
    i32 material_offset;
    float _pad0;
    Mat4 model_matrix;
};

constexpr i32 GRAPHICS_MAX_DRAW_INSTANCE_NUM = 1 << 17; // >100K

constexpr GLuint FRAME_CONSTANT_BIND_IDX = 0;
constexpr GLuint INSTANCE_BUF_BIND_IDX = 1;
constexpr GLuint MESH_VERTEX_BUF_BIND_IDX = 2;
constexpr GLuint MESH_INDEX_BUF_BIND_IDX = 3;
constexpr GLuint MATERIAL_BUF_BIND_IDX = 4;

struct DrawInstance {
    Vec3 position;
    Vec4 rotation;
    Vec3 scale;
    i32 mesh_idx;
    i32 material_idx;
};

struct GraphicsSys {
    GLuint program;
    GLuint instance_buf;
    GLuint frame_constant_uniform;

    GLuint iota_vbo;
    GLuint iota_vao;

    InstanceDataGPU *instance_buf_mapped;

    DrawInstance *draw_instance_list;
    i32 draw_instance_num;
    i32 draw_instance_cap;

    Vec2 view_size;
    Vec2 view_range;
    Vec3 view_eye;
    Vec3 view_center;
    Vec3 view_up;
    f32 view_fov;
};
inline GraphicsSys graphics_sys = {};

void graphics_sys_init(i32 draw_instance_cap = GRAPHICS_MAX_DRAW_INSTANCE_NUM) {
    // program
    graphics_sys.program = glCreateProgram();
    glAttachShader(graphics_sys.program, SHADER_VERTEX_WORLD_OBJECT);
    glAttachShader(graphics_sys.program, SHADER_FRAGMENT_WORLD_OBJECT);
    glLinkProgram(graphics_sys.program);

    GLint _linked = 0;
    glGetProgramiv(graphics_sys.program, GL_LINK_STATUS, &_linked);
    if (!_linked) {
        GLint log_len = 0;
        glGetProgramiv(graphics_sys.program, GL_INFO_LOG_LENGTH, &log_len);
        char *log = (char*)malloc(log_len);
        glGetProgramInfoLog(graphics_sys.program, log_len, nullptr, log);
        log_err("graphics_sys_init: program linking failed:\n%s", log);
        free(log);
    }

    // buffers
    glGenBuffers(1, &graphics_sys.instance_buf);
    glBindBuffer(GL_ARRAY_BUFFER, graphics_sys.instance_buf);
    glBufferData(GL_ARRAY_BUFFER, draw_instance_cap * sizeof(InstanceDataGPU), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &graphics_sys.frame_constant_uniform);
    glBindBuffer(GL_UNIFORM_BUFFER, graphics_sys.frame_constant_uniform);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(FrameConstantGPU), nullptr, GL_DYNAMIC_DRAW);

    // create a static vao that only contains 0 -> cap, which is handy for my render approach
    glGenBuffers(1, &graphics_sys.iota_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, graphics_sys.iota_vbo);
    i32 *iota_buf = (i32*)malloc(draw_instance_cap * sizeof(i32));
    for (i32 i = 0; i < draw_instance_cap; ++i) { iota_buf[i] = i; }
    glBufferData(GL_ARRAY_BUFFER, draw_instance_cap * sizeof(i32), iota_buf, GL_STATIC_DRAW);
    free(iota_buf);

    glGenVertexArrays(1, &graphics_sys.iota_vao);
    glBindVertexArray(graphics_sys.iota_vao);

    glBindBuffer(GL_ARRAY_BUFFER, graphics_sys.iota_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 1, GL_INT, sizeof(i32), (void*)0);
    glVertexAttribDivisor(0, 1);

    // depth
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // draw instances
    graphics_sys.instance_buf_mapped = (InstanceDataGPU*)arena_alloc(omni_arena_ptr, draw_instance_cap * sizeof(InstanceDataGPU));
    graphics_sys.draw_instance_list = (DrawInstance*)arena_alloc(omni_arena_ptr, draw_instance_cap * sizeof(DrawInstance));
    graphics_sys.draw_instance_cap = draw_instance_cap;
}

void graphics_draw() {
    glUseProgram(graphics_sys.program);
    glBindVertexArray(graphics_sys.iota_vao);

    // bind buffers
    glBindBufferBase(GL_UNIFORM_BUFFER, FRAME_CONSTANT_BIND_IDX, graphics_sys.frame_constant_uniform);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INSTANCE_BUF_BIND_IDX, graphics_sys.instance_buf);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_VERTEX_BUF_BIND_IDX, mesh_sys.mesh_vertex_buf);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_INDEX_BUF_BIND_IDX, mesh_sys.mesh_index_buf);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MATERIAL_BUF_BIND_IDX, material_sys.material_buf);

    // frame constant: view, projection
    Mat4 proj = mat4_perspective(
        graphics_sys.view_fov,
        graphics_sys.view_size.x / graphics_sys.view_size.y,
        graphics_sys.view_range.x,
        graphics_sys.view_range.y
    );

    Vec3 f = vec3_normalize(graphics_sys.view_center - graphics_sys.view_eye);
    Vec3 r = vec3_normalize(vec3_cross(f, graphics_sys.view_up));
    Vec3 u = vec3_cross(r, f);
    Mat4 view = MAT4_IDENTITY;
    view.raw[0][0] =  r.x; view.raw[1][0] =  r.y; view.raw[2][0] =  r.z; view.raw[3][0] = -vec3_dot(r, graphics_sys.view_eye);
    view.raw[0][1] =  u.x; view.raw[1][1] =  u.y; view.raw[2][1] =  u.z; view.raw[3][1] = -vec3_dot(u, graphics_sys.view_eye);
    view.raw[0][2] = -f.x; view.raw[1][2] = -f.y; view.raw[2][2] = -f.z; view.raw[3][2] =  vec3_dot(f, graphics_sys.view_eye);

    FrameConstantGPU frame_constant = {
        .view_matrix = view,
        .projection_matrix = proj,
        .eye_pos = graphics_sys.view_eye
    };

    // update buffers
    glBindBuffer(GL_UNIFORM_BUFFER, graphics_sys.frame_constant_uniform);
    glBufferSubData(
        GL_UNIFORM_BUFFER,
        0,
        sizeof(FrameConstantGPU),
        &frame_constant
    );
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // FIXME maybe this should be on GPU
    for (i32 i = 0; i < graphics_sys.draw_instance_num; i++) {
        DrawInstance draw_instance = graphics_sys.draw_instance_list[i];
        MeshMeta mesh_meta = mesh_get_meta(draw_instance.mesh_idx);
        graphics_sys.instance_buf_mapped[i] = {
            .mesh_vertex_offset = mesh_meta.mesh_vertex_offset,
            .mesh_index_offset = mesh_meta.mesh_index_offset,
            .material_offset = draw_instance.material_idx,
            .model_matrix = mat4_translation(draw_instance.position) *
                            mat4_rotation(draw_instance.rotation) *
                            mat4_scale(draw_instance.scale)
        };
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, graphics_sys.instance_buf);
    glBufferSubData(
        GL_SHADER_STORAGE_BUFFER,
        0,
        graphics_sys.draw_instance_num * sizeof(InstanceDataGPU),
        graphics_sys.instance_buf_mapped
    );
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // draw
    for (i32 i = 0; i < graphics_sys.draw_instance_num; i++) {
        DrawInstance draw_instance = graphics_sys.draw_instance_list[i];
        MeshMeta mesh_meta = mesh_get_meta(draw_instance.mesh_idx);

        // glDrawArrays(GL_TRIANGLES, 0, mesh_meta.indices_num);
        glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, mesh_meta.indices_num, 1, i);
    }
}

}
