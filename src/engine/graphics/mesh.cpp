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
import list;

export namespace asapage {

struct MeshKey {
    union {
        u64 raw;
        struct {
            i32 idx;
            u32 _reserve;
        };
    };
};

struct MeshMeta {
    i32 vertex_offset;
    i32 index_offset;
    i32 indices_num;
};

struct MeshVertexGPUMap {
    Vec3 pos;
    f32 _pad0;
    Vec3 normal;
    f32 _pad1;
    Vec2 uv;
    f32 _pad2;
    f32 _pad3;
};

constexpr usize MESH_VERTEX_BUF_SIZE_DEFAULT = 100 << 20; // 100MB
constexpr usize MESH_INDEX_BUF_SIZE_DEFAULT = 100 << 20; // 100MB
constexpr i32 MESH_CAP_DEFAULT = 1024;

struct MeshSys {
    GLuint vertex_gpu_buf;
    i32 vertex_gpu_buf_offset;

    GLuint index_gpu_buf;
    i32 index_gpu_buf_offset;

    List<MeshMeta> meta_list;
};
inline MeshSys mesh_sys = {};

void mesh_sys_init(i32 mesh_cap = MESH_CAP_DEFAULT,
                   usize mesh_vertex_buf_size = MESH_VERTEX_BUF_SIZE_DEFAULT,
                   usize mesh_index_buf_size = MESH_INDEX_BUF_SIZE_DEFAULT) {
    glGenBuffers(1, &mesh_sys.vertex_gpu_buf);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_sys.vertex_gpu_buf);
    glBufferData(GL_SHADER_STORAGE_BUFFER, mesh_vertex_buf_size, nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &mesh_sys.index_gpu_buf);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_sys.index_gpu_buf);
    glBufferData(GL_SHADER_STORAGE_BUFFER, mesh_index_buf_size, nullptr, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    list_init(&mesh_sys.meta_list, omni_arena_ptr, mesh_cap);

    // TODO make a stub that maybe like Valve's "ERROR"
}

MeshKey material_stub() { return MeshKey{0}; }

void mesh_load(const char *path, MeshKey *out_mesh_key_list) {
    cgltf_options load_model_options = {};
    cgltf_data *model_data = nullptr;

    if (cgltf_parse_file(&load_model_options, path, &model_data) != cgltf_result_success) {
        log_err("mesh_load(): failed to parse from path: path = [%s]", path);
        return;
    }
    if (cgltf_load_buffers(&load_model_options, model_data, path) != cgltf_result_success) {
        log_err("graphics_load_model: failed to load model buffer: path = [%s]", path);
        cgltf_free(model_data);
        return;
    }

    // a model can contains many meshes, so that loading a model can load many meshes
    for (cgltf_size _i = 0; _i < model_data->meshes_count; _i++) {
        cgltf_mesh *mesh = &model_data->meshes[_i];

        // calculate total vertex and index in a mesh
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

        // make buffers that will be map to GPU buffers
        auto *mesh_vertex_buf_mapped_buf = (MeshVertexGPUMap*)malloc(total_vertex_num * sizeof(MeshVertexGPUMap));
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
                MeshVertexGPUMap _vertex = {
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
        i32 mesh_vertex_offset = mesh_sys.vertex_gpu_buf_offset;
        usize mesh_vertex_buf_bytes_offset = mesh_vertex_offset * sizeof(MeshVertexGPUMap);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mesh_sys.vertex_gpu_buf);
        glBufferSubData(
            GL_SHADER_STORAGE_BUFFER,
            mesh_vertex_buf_bytes_offset,
            total_vertex_num * sizeof(MeshVertexGPUMap),
            mesh_vertex_buf_mapped_buf
        );
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        i32 mesh_index_offset = mesh_sys.index_gpu_buf_offset;
        usize mesh_index_buf_bytes_offset = mesh_index_offset * sizeof(i32);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_sys.index_gpu_buf);
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER,
            mesh_index_buf_bytes_offset,
            total_index_num * sizeof(i32),
            mesh_index_buf_mapped_buf
        );
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        free(mesh_vertex_buf_mapped_buf);
        free(mesh_index_buf_mapped_buf);

        //
        mesh_sys.vertex_gpu_buf_offset += total_vertex_num;
        mesh_sys.index_gpu_buf_offset += total_index_num;

        MeshMeta new_meta_data = { .vertex_offset = mesh_vertex_offset,
                                   .index_offset = mesh_index_offset,
                                   .indices_num = total_index_num };
        i32 new_meta_idx = list_create(&mesh_sys.meta_list, &new_meta_data);

        MeshKey key = { .idx = new_meta_idx };

        out_mesh_key_list[_i] = key;

        log_debug("Created Mesh [%d]: path = [%s]; vertex_num = [%d]; triangle_num = [%d]; vertex_offset = [%d]; index_offset = [%d]",
                  key.idx, path, total_vertex_num, total_index_num / 3, mesh_vertex_offset, mesh_index_offset);
    }

    cgltf_free(model_data);
}

MeshMeta mesh_get_meta(MeshKey key) {
    return *list_get(&mesh_sys.meta_list, key.idx);
}

}
