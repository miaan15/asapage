module;

#include <cstdlib>
#include <glad.h>

export module graphics;

export import graphics_shader;
export import graphics_object3d;
export import graphics_mesh;
export import graphics_material;

import define;
import mem;
import internal;
import math;
import log;

export namespace asapage {

struct FrameConstant {
    Vec2 view_size;
    Vec2 view_range;
    Vec3 view_eye;
    Vec3 view_center;
    Vec3 view_up;
    f32 view_fov;
};

struct alignas(16) FrameConstantGPUMap {
    Mat4 view_matrix;
    Mat4 projection_matrix;
    Vec3 eye_pos;
    f32 _pad0;
};

struct GraphicsSysDesc {
    i32 mesh_cap;
    usize mesh_vertex_buf_size;
    usize mesh_index_buf_size;

    i32 material_cap;

    i32 object_cap;
};

struct GraphicsSys {
    GLuint gpu_program;
    GLuint frame_constant_gpu_uniform;

    // a vbo that contains array of ints 0->MAX, use for pass object_idx (which use to access object data) to shader
    // because gl 4.3 does not has 'that' functionality yet (or I thought that)
    GLuint iota_vao;
    GLuint iota_vbo;

    FrameConstant frame_constant;
    FrameConstantGPUMap frame_constant_gpu_map;
};
inline GraphicsSys graphics_sys = {};

// storage buffer/uniform location in shader
constexpr GLuint FRAME_CONSTANT_UNIFORM_BIND_IDX = 0;
constexpr GLuint OBJECT_LIST_BIND_IDX = 1;
constexpr GLuint MESH_VERTEX_BUF_BIND_IDX = 2;
constexpr GLuint MESH_INDEX_BUF_BIND_IDX = 3;
constexpr GLuint MATERIAL_LIST_BIND_IDX = 4;

void graphics_sys_init(GraphicsSysDesc *desc = nullptr) {
    shader_sys_init(); // FIXME shader is a different shiet here, but for the sake of simplicity for now, I put it here

    if (desc != nullptr) {
        mesh_sys_init(desc->mesh_cap, desc->mesh_vertex_buf_size, desc->mesh_index_buf_size);
        material_sys_init(desc->material_cap);
        object3d_sys_init(desc->object_cap);
    } else {
        mesh_sys_init();
        material_sys_init();
        object3d_sys_init();
    }

    // program, link with shaders
    graphics_sys.gpu_program = glCreateProgram();
    glAttachShader(graphics_sys.gpu_program, SHADER_VERTEX_WORLD_OBJECT);
    glAttachShader(graphics_sys.gpu_program, SHADER_FRAGMENT_WORLD_OBJECT);
    glLinkProgram(graphics_sys.gpu_program);

    GLint _linked = 0;
    glGetProgramiv(graphics_sys.gpu_program, GL_LINK_STATUS, &_linked);
    if (!_linked) {
        GLint log_len = 0;
        glGetProgramiv(graphics_sys.gpu_program, GL_INFO_LOG_LENGTH, &log_len);
        char *log = (char*)malloc(log_len);
        glGetProgramInfoLog(graphics_sys.gpu_program, log_len, nullptr, log);
        log_err("graphics_sys_init(): program linking failed:\n%s", log);
        free(log);
    }

    //
    glGenBuffers(1, &graphics_sys.frame_constant_gpu_uniform);
    glBindBuffer(GL_UNIFORM_BUFFER, graphics_sys.frame_constant_gpu_uniform); {
        glBufferData(GL_UNIFORM_BUFFER, sizeof(FrameConstantGPUMap), nullptr, GL_DYNAMIC_DRAW);
    } glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // iota vbo
    glGenBuffers(1, &graphics_sys.iota_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, graphics_sys.iota_vbo); {
        i32 cap = object3d_sys.object_pool.cap;

        i32 *iota_buf = (i32*)malloc(cap * sizeof(i32));
        for (i32 i = 0; i < cap; ++i) { iota_buf[i] = i; }

        glBufferData(GL_ARRAY_BUFFER, cap * sizeof(i32), iota_buf, GL_STATIC_DRAW);

        free(iota_buf);
    } glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenVertexArrays(1, &graphics_sys.iota_vao);
    glBindVertexArray(graphics_sys.iota_vao); {
        glBindBuffer(GL_ARRAY_BUFFER, graphics_sys.iota_vbo);
        glVertexAttribIPointer(0, 1, GL_INT, sizeof(i32), (void*)0);
        glVertexAttribDivisor(0, 1);
        glEnableVertexAttribArray(0);
    } glBindVertexArray(0);

    // enable gl stuffs
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void _graphics_update_frame();
void _graphics_upload_to_gpu();

void graphics_draw() {
    // bind buffers
    glBindBufferBase(GL_UNIFORM_BUFFER, FRAME_CONSTANT_UNIFORM_BIND_IDX, graphics_sys.frame_constant_gpu_uniform);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, OBJECT_LIST_BIND_IDX, object3d_sys.object_gpu_list);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_VERTEX_BUF_BIND_IDX, mesh_sys.vertex_gpu_buf);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MESH_INDEX_BUF_BIND_IDX, mesh_sys.index_gpu_buf);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MATERIAL_LIST_BIND_IDX, material_sys.material_gpu_list);

    _graphics_update_frame();
    _graphics_upload_to_gpu();

    // draw
    glUseProgram(graphics_sys.gpu_program);
    glBindVertexArray(graphics_sys.iota_vao); {
        for (i32 i = 0; i < object3d_sys.draw_object_num; i++) {
            i32 indices_num = *list_get(&object3d_sys.draw_object_indices_num_list, i);
            glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, indices_num, 1, i);
        }
    } glBindVertexArray(0);
      glUseProgram(0);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) { log_err("GL error: %d", err); }
}

void graphics_update_tick() {
    object3d_update_tick();
}

void _graphics_update_frame() {
    // frame constant
    FrameConstant frame_constant = graphics_sys.frame_constant;

    Mat4 proj = mat4_perspective(
        frame_constant.view_fov,
        frame_constant.view_size.x / frame_constant.view_size.y,
        frame_constant.view_range.x,
        frame_constant.view_range.y
    );

    Vec3 f = vec3_normalize(frame_constant.view_center - frame_constant.view_eye);
    Vec3 r = vec3_normalize(vec3_cross(f, frame_constant.view_up));
    Vec3 u = vec3_cross(r, f);
    Mat4 view = MAT4_IDENTITY;
    view.raw[0][0] =  r.x; view.raw[1][0] =  r.y; view.raw[2][0] =  r.z; view.raw[3][0] = -vec3_dot(r, frame_constant.view_eye);
    view.raw[0][1] =  u.x; view.raw[1][1] =  u.y; view.raw[2][1] =  u.z; view.raw[3][1] = -vec3_dot(u, frame_constant.view_eye);
    view.raw[0][2] = -f.x; view.raw[1][2] = -f.y; view.raw[2][2] = -f.z; view.raw[3][2] =  vec3_dot(f, frame_constant.view_eye);

    graphics_sys.frame_constant_gpu_map = {
        .view_matrix = view,
        .projection_matrix = proj,
        .eye_pos = frame_constant.view_eye
    };

    //
    material_update_frame();
    object3d_update_frame();
}

void _graphics_upload_to_gpu() {
    // frame constant
    glBindBuffer(GL_UNIFORM_BUFFER, graphics_sys.frame_constant_gpu_uniform); {
        glBufferSubData(GL_UNIFORM_BUFFER,
                        0, sizeof(FrameConstantGPUMap),
                        &graphics_sys.frame_constant_gpu_map);
    } glBindBuffer(GL_UNIFORM_BUFFER, 0);

    //
    material_upload_to_gpu();
    object3d_upload_to_gpu();
}

}
