module;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <SDL3/SDL_gpu.h>
#include <cgltf/cgltf.h>

export module graphics_model;

import define;
import mem;
import file;
import internal;

export namespace asapage {

void upload_data_to_gpu_buf(SDL_GPUBuffer *gpu_buf, void *data_buf, i32 data_buf_size) {
    SDL_GPUTransferBufferCreateInfo transfer_buf_cinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = (u32)data_buf_size
    };
    SDL_GPUTransferBuffer *transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);

    void *transfer_buf_mapped_ptr = SDL_MapGPUTransferBuffer(gpu_device, transfer_buf, false);
    mem_copy(transfer_buf_mapped_ptr, data_buf, data_buf_size);
    SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buf);

    SDL_GPUCommandBuffer* upload_command_buf = SDL_AcquireGPUCommandBuffer(gpu_device); {
        SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_command_buf); {
            SDL_GPUTransferBufferLocation transfer_buf_location = {
                .transfer_buffer = transfer_buf,
                .offset = 0
            };
            SDL_GPUBufferRegion buf_region = {
                .buffer = gpu_buf,
                .offset = 0,
                .size = (u32)data_buf_size
            };
            SDL_UploadToGPUBuffer(copy_pass, &transfer_buf_location, &buf_region, false);
        } SDL_EndGPUCopyPass(copy_pass);
    } SDL_SubmitGPUCommandBuffer(upload_command_buf);

    SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buf);
}

i32 model_load(SDL_GPUBuffer **vertices_buf, SDL_GPUBuffer **normals_buf, SDL_GPUBuffer **indices_buf, const char *path) {
    char true_path[256];
    strcpy(true_path, path);
    file_to_asset_path(true_path, 256);

    cgltf_options load_model_options = {};
    cgltf_data *model_data;

    if (cgltf_parse_file(&load_model_options, true_path, &model_data) != cgltf_result_success) {
        printf("failed to parse model: path = [%s]\n", true_path);
        return -1;
    }
    if (cgltf_load_buffers(&load_model_options, model_data, true_path) != cgltf_result_success) {
        printf("failed to load model buffer: path = [%s]\n", true_path);
        cgltf_free(model_data);
        return -1;
    }

    i32 total_indices_num = 0;
    for (i32 i = 0; i < model_data->meshes_count; i++) {
        cgltf_mesh *mesh = &model_data->meshes[i];
        for (i32 j = 0; j < mesh->primitives_count; j++) {
            cgltf_primitive *primitive = &mesh->primitives[j];

            // vertices
            cgltf_accessor *vertices_accessor = nullptr;
            for (cgltf_size k = 0; k < primitive->attributes_count; k++) {
                if (primitive->attributes[k].type == cgltf_attribute_type_position) {
                    vertices_accessor = primitive->attributes[k].data;
                    break;
                }
            }
            if (vertices_accessor == nullptr) {
                printf("somehow the model has no vertices??? %s\n", true_path);
                return -1;
            }

            i32 vertices_num = vertices_accessor->count;
            i32 vertices_data_buf_size = 3 * vertices_num * sizeof(float);
            float *vertices_data_buf = (float*)malloc(vertices_data_buf_size);

            cgltf_accessor_unpack_floats(vertices_accessor, vertices_data_buf, 3 * vertices_num);

            // indices
            cgltf_accessor *indices_accessor = primitive->indices;
            if (indices_accessor == nullptr) {
                printf("somehow the model has no indices??? %s\n", true_path);
                return -1;
            }

            i32 indices_num = indices_accessor->count;
            i32 indices_data_buf_size = indices_num * sizeof(i32);
            int *indices_data_buf = (i32*)malloc(indices_data_buf_size);

            cgltf_accessor_unpack_indices(indices_accessor, indices_data_buf, sizeof(i32), indices_num);

            // normals
            cgltf_accessor *normals_accessor = nullptr;
            for (cgltf_size k = 0; k < primitive->attributes_count; k++) {
                if (primitive->attributes[k].type == cgltf_attribute_type_normal) {
                    normals_accessor = primitive->attributes[k].data;
                    break;
                }
            }
            if (normals_accessor == nullptr) {
                printf("ok this sh t has no normals? %s", true_path);
                return -1;
            }

            i32 normals_num = normals_accessor->count;
            i32 normals_data_buf_size = 3 * normals_num * sizeof(float);
            float *normals_data_buf = (float*)malloc(normals_data_buf_size);

            cgltf_accessor_unpack_floats(normals_accessor, normals_data_buf, 3 * normals_num);

            // FIXME only one mesh, this will overwrite other if this model has more than 1 mesh
            // upload to GPU buf
            upload_data_to_gpu_buf(*vertices_buf, vertices_data_buf, vertices_data_buf_size);
            upload_data_to_gpu_buf(*indices_buf, indices_data_buf, indices_data_buf_size);
            upload_data_to_gpu_buf(*normals_buf, normals_data_buf, normals_data_buf_size);

            free(vertices_data_buf);
            free(indices_data_buf);
            free(normals_data_buf);

            total_indices_num += indices_num;
        }
    }

    cgltf_free(model_data);

    return total_indices_num;
}

// FIXME maybe change a lot
struct Transform {
    f32 translation[3];
    f32 rotation[4];
    f32 scale[3];
};
void init_pipeline(SDL_GPUGraphicsPipeline **pipeline, SDL_GPUShader *vertex_shader, SDL_GPUShader *fragment_shader) {
    SDL_GPUGraphicsPipelineCreateInfo pipeline_cinfo = {};
    pipeline_cinfo.vertex_shader = vertex_shader;
    pipeline_cinfo.fragment_shader = fragment_shader;
    pipeline_cinfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    // vertex input state
    SDL_GPUVertexBufferDescription vertex_buffer_decs_list_buf[3] = {}; {
        vertex_buffer_decs_list_buf[0] = { // vertex pos
            .slot = 0,
            .pitch = 3 * sizeof(f32),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        };
        vertex_buffer_decs_list_buf[1] = { // vertex normal
            .slot = 1,
            .pitch = 3 * sizeof(f32),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        };
        vertex_buffer_decs_list_buf[2] = { // index
            .slot = 2,
            .pitch = sizeof(Transform),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
        };
    }

    SDL_GPUVertexAttribute vertex_attribute_list_buf[5] = {}; {
        vertex_attribute_list_buf[0] = { // layout(location = 0) in vec3 i_pos;
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0,
        };
        vertex_attribute_list_buf[1] = { // layout(location = 1) in vec3 i_normal;
            .location = 1,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0,
        };
        vertex_attribute_list_buf[2] = { // layout(location = 4) in vec3 i_translation;
            .location = 4,
            .buffer_slot = 2,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Transform, translation),
        };
        vertex_attribute_list_buf[3] = { // layout(location = 5) in vec4 i_rotation;
            .location = 5,
            .buffer_slot = 2,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(Transform, rotation),
        };
        vertex_attribute_list_buf[4] = { // layout(location = 6) in vec3 i_scale;
            .location = 6,
            .buffer_slot = 2,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Transform, scale),
        };
    }

    SDL_GPUVertexInputState vertex_input_state = {
        .vertex_buffer_descriptions = vertex_buffer_decs_list_buf, .num_vertex_buffers = 3,
        .vertex_attributes = vertex_attribute_list_buf, .num_vertex_attributes = 5,
    };
    pipeline_cinfo.vertex_input_state = vertex_input_state;

    // pipeline target info
    SDL_GPUColorTargetDescription color_target_desc_list_buf[1] = {}; {
        color_target_desc_list_buf[0] = {
            .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window), // FIXME
        };
    }

    SDL_GPUGraphicsPipelineTargetInfo pipeline_target_info = {
        .color_target_descriptions = color_target_desc_list_buf, .num_color_targets = 1,
        .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT, .has_depth_stencil_target = true
    };
    pipeline_cinfo.target_info = pipeline_target_info;

    // rasterizer
    SDL_GPURasterizerState rasterizer_state = {
        .cull_mode = SDL_GPU_CULLMODE_BACK,
        .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
    };
    pipeline_cinfo.rasterizer_state = rasterizer_state;

    // depth stencil
    SDL_GPUDepthStencilState depth_stencil_state = {
        .compare_op = SDL_GPU_COMPAREOP_LESS,
        .enable_depth_test = true,
        .enable_depth_write = true
    };
    pipeline_cinfo.depth_stencil_state = depth_stencil_state;

    *pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_cinfo);
}

}
