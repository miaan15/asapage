module;

#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#include <glad.h>

export module graphics_mesh;

import define;
import mem;
import internal;
import file;
import log;
import math;

export namespace asapage {

struct MeshVertexGPU {
    Vec3 pos;
    float _pad0;
    Vec3 normal;
    float _pad1;
    Vec2 uv;
    Vec2 _pad2;
};

constexpr i32 MESH_MAX_MESH_NUM = 1024;
constexpr usize MESH_MAX_MESH_VERTEX_BUF_SIZE = 100 << 20; // 100MB
constexpr usize MESH_MAX_MESH_INDEX_BUF_SIZE = 100 << 20; // 100MB

struct MeshMeta {
    i32 mesh_vertex_offset;
    i32 mesh_index_offset;
    i32 indices_num;
};

struct MeshSys {
    GLuint mesh_vertex_buf;
    GLuint mesh_index_buf;

    i32 loaded_mesh_vertex_num;
    i32 loaded_mesh_index_num;

    MeshMeta *mesh_meta_list;
    i32 mesh_num;
    i32 mesh_cap;
};
inline MeshSys mesh_sys = {};

MeshMeta mesh_get_meta(i32 idx) {
    return mesh_sys.mesh_meta_list[idx];
}

void mesh_sys_init(i32 mesh_cap = MESH_MAX_MESH_NUM,
                   usize max_mash_vertex_buf_size = MESH_MAX_MESH_VERTEX_BUF_SIZE,
                   usize max_mash_index_buf_size = MESH_MAX_MESH_INDEX_BUF_SIZE) {
    glGenBuffers(1, &mesh_sys.mesh_vertex_buf);
    glBindBuffer(GL_ARRAY_BUFFER, mesh_sys.mesh_vertex_buf);
    glBufferData(GL_ARRAY_BUFFER, max_mash_vertex_buf_size, nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &mesh_sys.mesh_index_buf);
    glBindBuffer(GL_ARRAY_BUFFER, mesh_sys.mesh_index_buf);
    glBufferData(GL_ARRAY_BUFFER, max_mash_index_buf_size, nullptr, GL_DYNAMIC_DRAW);

    mesh_sys.mesh_meta_list = (MeshMeta*)arena_alloc(omni_arena_ptr, mesh_cap * sizeof(MeshMeta));
    mesh_sys.mesh_cap = mesh_cap;

    // stub
    mesh_sys.mesh_num = 1;
    // TODO a stub mesh (idk, maybe a big 'ERROR' like valve)
}

void mesh_load(const char *path) {
    cgltf_options load_model_options = {};
    cgltf_data *model_data = nullptr;

    if (cgltf_parse_file(&load_model_options, path, &model_data) != cgltf_result_success) {
        log_err("mesh load: failed to parse: path = [%s]", path);
        return;
    }
    if (cgltf_load_buffers(&load_model_options, model_data, path) != cgltf_result_success) {
        log_err("graphics_load_model: failed to load model buffer: path = [%s]", path);
        cgltf_free(model_data);
        return;
    }

    // a model can contains many meshes
    for (cgltf_size _i = 0; _i < model_data->meshes_count; _i++) {
        if (mesh_sys.mesh_num >= mesh_sys.mesh_cap) {
            log_err("mesh_load: too many mesh: num = [%d]", mesh_sys.mesh_num);
            return;
        }

        cgltf_mesh *mesh = &model_data->meshes[_i];

        // cal total vertex and index in a mesh
        i32 total_vertex_num = 0;
        i32 total_index_num = 0;
        for (cgltf_size i = 0; i < mesh->primitives_count; i++) {
            cgltf_primitive *primitive = &mesh->primitives[i];

            for (cgltf_size j = 0; j < primitive->attributes_count; j++) {
                if (primitive->attributes[j].type == cgltf_attribute_type_position) {
                    total_vertex_num += primitive->attributes[j].data->count;
                    break;
                }
            }

            if (primitive->indices != nullptr) {
                total_index_num += primitive->indices->count;
            }
        }

        // buffer that map to GPU buffer
        auto *mesh_vertex_buf_mapped_buf = (MeshVertexGPU*)malloc(total_vertex_num * sizeof(MeshVertexGPU));
        auto *mesh_index_buf_mapped_buf = (i32*)malloc(total_index_num * sizeof(i32));

        // populate map buffer
        i32 mesh_vertex_buf_mapped_buf_offset = 0;
        i32 mesh_index_buf_mapped_buf_offset = 0;
        i32 vertex_index_offset = 0;
        for (cgltf_size i = 0; i < mesh->primitives_count; i++) {
            cgltf_primitive *primitive = &mesh->primitives[i];

            cgltf_accessor *pos_accessor = nullptr;
            cgltf_accessor *normal_accessor = nullptr;
            cgltf_accessor *uv_accessor = nullptr;
            cgltf_accessor *index_accessor = primitive->indices;

            for (cgltf_size j = 0; j < primitive->attributes_count; j++) {
                cgltf_attribute *attribute = &primitive->attributes[j];
                switch (attribute->type) {
                case cgltf_attribute_type_position: pos_accessor = attribute->data; break;
                case cgltf_attribute_type_normal:   normal_accessor = attribute->data; break;
                case cgltf_attribute_type_texcoord: uv_accessor = attribute->data; break;
                default: break;
                }
            }

            if (!pos_accessor) { continue; }

            // there might be no normal/uv/index, by that, keep the default value
            i32 vertices_num = pos_accessor->count;
            for (i32 v = 0; v < vertices_num; v++) {
                MeshVertexGPU _vertex = {
                    .pos = { 0, 0, 0 },
                    .normal = { 0, 1, 0 },
                    .uv = { 0, 0 }
                };

                cgltf_accessor_read_float(pos_accessor, v, _vertex.pos.raw, 3);
                if (normal_accessor != nullptr) { cgltf_accessor_read_float(normal_accessor, v, _vertex.normal.raw, 3); }
                if (uv_accessor != nullptr) { cgltf_accessor_read_float(uv_accessor, v, _vertex.uv.raw, 2); }

                mesh_vertex_buf_mapped_buf[mesh_vertex_buf_mapped_buf_offset + v] = _vertex;
            }

            i32 indices_num = index_accessor ? index_accessor->count : 0;
            if (index_accessor != nullptr) {
                for (i32 k = 0; k < indices_num; k++) {
                    cgltf_size idx = cgltf_accessor_read_index(index_accessor, k);
                    mesh_index_buf_mapped_buf[mesh_index_buf_mapped_buf_offset + k] = idx + vertex_index_offset;
                }
            }

            mesh_vertex_buf_mapped_buf_offset += vertices_num;
            mesh_index_buf_mapped_buf_offset += indices_num;

            vertex_index_offset += vertices_num;
        }

        // upload to GPU
        i32 mesh_vertex_offset = mesh_sys.loaded_mesh_vertex_num;
        usize mesh_vertex_buf_bytes_offset = mesh_vertex_offset * sizeof(MeshVertexGPU);
        glBindBuffer(GL_ARRAY_BUFFER, mesh_sys.mesh_vertex_buf);
        glBufferSubData(
            GL_ARRAY_BUFFER,
            mesh_vertex_buf_bytes_offset,
            total_vertex_num * sizeof(MeshVertexGPU),
            mesh_vertex_buf_mapped_buf
        );

        i32 mesh_index_offset = mesh_sys.loaded_mesh_index_num;
        usize mesh_index_buf_bytes_offset = mesh_index_offset * sizeof(i32);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_sys.mesh_index_buf);
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            mesh_index_buf_bytes_offset,
            total_index_num * sizeof(i32),
            mesh_index_buf_mapped_buf
        );

        free(mesh_vertex_buf_mapped_buf);
        free(mesh_index_buf_mapped_buf);

        i32 mesh_idx = mesh_sys.mesh_num;

        mesh_sys.mesh_meta_list[mesh_idx] = {
            .mesh_vertex_offset = mesh_vertex_offset,
            .mesh_index_offset  = mesh_index_offset,
            .indices_num        = total_index_num
        };

        mesh_sys.loaded_mesh_vertex_num += total_vertex_num;
        mesh_sys.loaded_mesh_index_num += total_index_num;

        mesh_sys.mesh_num++;

        log_debug("Load mesh [%d]: path = [%s]; vertex_num = [%d]; triangle_num = [%d]; vertex_offset = [%d]; index_offset = [%d]",
                  mesh_idx, path, total_vertex_num, total_index_num / 3, mesh_vertex_offset, mesh_index_offset);
    }

    cgltf_free(model_data);
}

}
