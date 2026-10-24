module;

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <SDL3/SDL_gpu.h>
#include <cgltf/cgltf.h>

export module graphics;

import define;
import mem;
import internal;
import context;
import math;
import file;
import shader;
import pool;

export namespace asapage {

struct alignas(16) GraphicsPerIntanceAccessor {
    u32 model_idx;
    u32 transform_idx;
    u32 color_idx;
};

struct alignas(16) GraphicsPerVertexAccessor {
};

struct alignas(16) GraphicsModelHeader {
    u32 vertices_offset;
    u32 normals_offset;
    u32 indices_offset;
};

struct alignas(16) GraphicsTransform {
    Vec3 position;
    f32 __padding_0;
    Vec4 rotation;
    Vec3 scale;
    f32 __padding_1;
};

struct alignas(16) GraphicsColor {
    Vec3 color;
    f32 __padding_0;
};

struct alignas(16) GraphicsView {
    Vec2 size;
    Vec2 range;
    Vec3 eye;
    f32 __padding_0;
    Vec3 center;
    f32 __padding_1;
    Vec3 up;
    f32 fov;
};

struct alignas(16) GraphicsEnv {
    Vec3 light_pos;
    f32 light_intensity;
};

constexpr i32 GRAPHICS_MAX_OBJECT_NUM = 1e4; // 10k
constexpr i32 GRAPHICS_MAX_MODEL_NUM = 100;

constexpr i32 GRAPHICS_VERTICES_BUF_SIZE = 1 << 20; // 1MB
constexpr i32 GRAPHICS_INDICES_BUF_SIZE = 1 << 20; // 1MB

constexpr i32 GRAPHICS_VERTEX_SHADER_BUF_NUM = 8;
constexpr i32 GRAPHICS_FRAGMENT_SHADER_BUF_NUM = 5;

struct GraphicsObject {
    i32 idx;

    i32 model_idx;

    Vec3 position;
    Quat rotation;
    Vec3 scale;

    Vec3 color;
};

struct GraphicsModelData {
    i32 indices_num;
};

struct GraphicsDrawCallData {
    i32 model_idx;
};

struct GraphicsSys {
    SDL_GPUTextureFormat screen_format;

    SDL_GPUTextureFormat depth_format;
    SDL_GPUTexture *depth_texture;

    SDL_GPUGraphicsPipeline *pipeline;

    SDL_GPUBuffer *per_instance_accessor_buf;
    SDL_GPUBuffer *per_vertex_accessor_buf;
    SDL_GPUBuffer *model_header_buf;
    SDL_GPUBuffer *transform_buf;
    SDL_GPUBuffer *color_buf;
    SDL_GPUBuffer *view_buf;
    SDL_GPUBuffer *env_buf;

    SDL_GPUBuffer *vertices_buf;
    SDL_GPUBuffer *normals_buf;
    SDL_GPUBuffer *indices_buf;

    //
    Pool<GraphicsObject> graphics_object_pool;

    i32 model_data_list_len;
    GraphicsModelData *model_data_list_buf;

    i32 draw_call_data_len;
    GraphicsDrawCallData *draw_call_data_buf;

    //
    GraphicsView view;
    GraphicsEnv env;

    //
    i32 vertices_buf_offset;
    i32 normals_buf_offset;
    i32 indices_buf_offset;
};
inline GraphicsSys graphics_sys = {};

void graphics_handle_create_buf(SDL_GPUBuffer **buf, i32 size) {
    SDL_GPUBufferCreateInfo buf_cinfo = {
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = (u32)size
    };
    *buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);
}

void graphics_sys_init() {
    graphics_sys.screen_format = SDL_GetGPUSwapchainTextureFormat(gpu_device, sdl_window);
    graphics_sys.depth_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

    { // depth_texture
        SDL_GPUTextureCreateInfo texture_cinfo = {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
            .width = (u32)screen_width,
            .height = (u32)screen_height,
            .layer_count_or_depth = 1,
            .num_levels = 1
        };
        graphics_sys.depth_texture = SDL_CreateGPUTexture(gpu_device, &texture_cinfo);
    }

    { // pipeline
        // shaders
        SDL_GPUShader *vertex_shader = shader_load("shader_vertex.hlsl", SHADER_STAGE::VERTEX, 0, 0, GRAPHICS_VERTEX_SHADER_BUF_NUM, 0);
        SDL_GPUShader *fragment_shader = shader_load("shader_fragment.hlsl", SHADER_STAGE::FRAGMENT, 0, 0, GRAPHICS_FRAGMENT_SHADER_BUF_NUM, 0);

        //
        SDL_GPUGraphicsPipelineCreateInfo pipeline_cinfo = {};
        pipeline_cinfo.vertex_shader = vertex_shader;
        pipeline_cinfo.fragment_shader = fragment_shader;
        pipeline_cinfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // pipeline target info
        SDL_GPUColorTargetDescription color_target_desc_list_buf[1] = {}; {
            color_target_desc_list_buf[0] = {
                .format = graphics_sys.screen_format
            };
        }
        SDL_GPUGraphicsPipelineTargetInfo pipeline_target_info = {
            .color_target_descriptions = color_target_desc_list_buf,
            .num_color_targets = 1,
            .depth_stencil_format = graphics_sys.depth_format,
            .has_depth_stencil_target = true
        };
        pipeline_cinfo.target_info = pipeline_target_info;

        // rasterizer
        SDL_GPURasterizerState rasterizer_state = {
            .cull_mode = SDL_GPU_CULLMODE_NONE,
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

        // submit
        graphics_sys.pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_cinfo);

        SDL_ReleaseGPUShader(gpu_device, vertex_shader);
        SDL_ReleaseGPUShader(gpu_device, fragment_shader);
    }

    // buffers
    graphics_handle_create_buf(&graphics_sys.per_instance_accessor_buf, GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsPerIntanceAccessor));
    graphics_handle_create_buf(&graphics_sys.per_vertex_accessor_buf, 3 * GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsPerVertexAccessor));
    graphics_handle_create_buf(&graphics_sys.model_header_buf, GRAPHICS_MAX_MODEL_NUM * sizeof(GraphicsModelHeader));
    graphics_handle_create_buf(&graphics_sys.transform_buf, GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsTransform));
    graphics_handle_create_buf(&graphics_sys.color_buf, GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsColor));
    graphics_handle_create_buf(&graphics_sys.view_buf, sizeof(GraphicsView));
    graphics_handle_create_buf(&graphics_sys.env_buf, sizeof(GraphicsEnv));

    graphics_handle_create_buf(&graphics_sys.vertices_buf, GRAPHICS_VERTICES_BUF_SIZE);
    graphics_handle_create_buf(&graphics_sys.normals_buf, GRAPHICS_VERTICES_BUF_SIZE);
    graphics_handle_create_buf(&graphics_sys.indices_buf, GRAPHICS_INDICES_BUF_SIZE);

    // pool
    pool_init(&graphics_sys.graphics_object_pool, omni_arena_ptr, GRAPHICS_MAX_OBJECT_NUM, offsetof(GraphicsObject, idx));

    // model data
    graphics_sys.model_data_list_len = 0;
    graphics_sys.model_data_list_buf =
        (GraphicsModelData*)arena_alloc(omni_arena_ptr, GRAPHICS_MAX_MODEL_NUM * sizeof(GraphicsModelData));

    // draw call data
    graphics_sys.draw_call_data_len = 0;
    graphics_sys.draw_call_data_buf =
        (GraphicsDrawCallData*)arena_alloc(omni_arena_ptr, GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsDrawCallData));
}

void graphics_sys_destroy() {
    SDL_ReleaseGPUTexture(gpu_device, graphics_sys.depth_texture);

    SDL_ReleaseGPUGraphicsPipeline(gpu_device, graphics_sys.pipeline);

    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.per_instance_accessor_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.per_vertex_accessor_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.model_header_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.transform_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.color_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.view_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.env_buf);

    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.vertices_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.normals_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.indices_buf);
}

bool graphics_draw() {
    u32 per_instance_size = GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsPerIntanceAccessor);
    u32 per_vertex_size = 3 * GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsPerVertexAccessor);
    u32 transform_transfer_size = GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsTransform);
    u32 color_transfer_size = GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsColor);
    u32 view_transfer_size = sizeof(GraphicsView);
    u32 env_transfer_size = sizeof(GraphicsEnv);

    // upload all stuff to GPU
    SDL_GPUCommandBuffer *upload_command_buf = SDL_AcquireGPUCommandBuffer(gpu_device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_command_buf);

    // setup transfers
    SDL_GPUTransferBuffer *per_instance_transfer_buf;
    SDL_GPUTransferBuffer *per_vertex_transfer_buf;
    SDL_GPUTransferBuffer *transform_transfer_buf;
    SDL_GPUTransferBuffer *color_transfer_buf;
    SDL_GPUTransferBuffer *view_transfer_buf;
    SDL_GPUTransferBuffer *env_transfer_buf;

    SDL_GPUTransferBufferCreateInfo transfer_buf_cinfo = { .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD };

    transfer_buf_cinfo.size = per_instance_size;
    per_instance_transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);

    transfer_buf_cinfo.size = per_vertex_size;
    per_vertex_transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);

    transfer_buf_cinfo.size = transform_transfer_size;
    transform_transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);

    transfer_buf_cinfo.size = color_transfer_size;
    color_transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);

    transfer_buf_cinfo.size = view_transfer_size;
    view_transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);

    transfer_buf_cinfo.size = env_transfer_size;
    env_transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);

    // map data
    auto *per_instance_ptr = (GraphicsPerIntanceAccessor*)SDL_MapGPUTransferBuffer(gpu_device, per_instance_transfer_buf, false);
    auto *per_vertex_ptr   = (GraphicsPerVertexAccessor*)SDL_MapGPUTransferBuffer(gpu_device, per_vertex_transfer_buf, false);
    auto *transform_ptr    = (GraphicsTransform*)SDL_MapGPUTransferBuffer(gpu_device, transform_transfer_buf, false);
    auto *color_ptr        = (GraphicsColor*)SDL_MapGPUTransferBuffer(gpu_device, color_transfer_buf, false);
    auto *view_ptr         = (GraphicsView*)SDL_MapGPUTransferBuffer(gpu_device, view_transfer_buf, false);
    auto *env_ptr          = (GraphicsEnv*)SDL_MapGPUTransferBuffer(gpu_device, env_transfer_buf, false);

    i32 object_idx = 0;
    GraphicsObject *object = nullptr;
    while (pool_iterate(&graphics_sys.graphics_object_pool, &object_idx, &object, true)) {
        transform_ptr[object_idx].position = object->position;
        transform_ptr[object_idx].rotation = object->rotation;
        transform_ptr[object_idx].scale = object->scale;

        color_ptr[object_idx].color = object->color;

        if (!pool_is_alive(&graphics_sys.graphics_object_pool, object)) { continue; }

        i32 draw_idx = graphics_sys.draw_call_data_len;

        graphics_sys.draw_call_data_buf[draw_idx] = (GraphicsDrawCallData) {
            .model_idx = object->model_idx
        };

        per_instance_ptr[draw_idx] = (GraphicsPerIntanceAccessor) {
            .model_idx = (u32)object->model_idx,
            .transform_idx = (u32)object_idx,
            .color_idx = (u32)object_idx
        };

        graphics_sys.draw_call_data_len++;
    }
    mem_copy(view_ptr, &graphics_sys.view, sizeof(GraphicsView));
    mem_copy(env_ptr, &graphics_sys.env, sizeof(GraphicsEnv));

    SDL_UnmapGPUTransferBuffer(gpu_device, per_instance_transfer_buf);
    SDL_UnmapGPUTransferBuffer(gpu_device, per_vertex_transfer_buf);
    SDL_UnmapGPUTransferBuffer(gpu_device, transform_transfer_buf);
    SDL_UnmapGPUTransferBuffer(gpu_device, color_transfer_buf);
    SDL_UnmapGPUTransferBuffer(gpu_device, view_transfer_buf);
    SDL_UnmapGPUTransferBuffer(gpu_device, env_transfer_buf);

    // upload transfer
    auto upload_buffer = [](SDL_GPUCopyPass *copy_pass, SDL_GPUTransferBuffer *src_transfer_buf, SDL_GPUBuffer *dst_buf, u32 size) {
        SDL_GPUTransferBufferLocation src_location = { .transfer_buffer = src_transfer_buf, .offset = 0 };
        SDL_GPUBufferRegion dst_region = { .buffer = dst_buf, .offset = 0, .size = size };
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dst_region, false);
    };

    upload_buffer(copy_pass, per_instance_transfer_buf, graphics_sys.per_instance_accessor_buf, per_instance_size);
    upload_buffer(copy_pass, per_vertex_transfer_buf, graphics_sys.per_vertex_accessor_buf, per_vertex_size);
    upload_buffer(copy_pass, transform_transfer_buf, graphics_sys.transform_buf, transform_transfer_size);
    upload_buffer(copy_pass, color_transfer_buf, graphics_sys.color_buf, color_transfer_size);
    upload_buffer(copy_pass, view_transfer_buf, graphics_sys.view_buf, view_transfer_size);
    upload_buffer(copy_pass, env_transfer_buf, graphics_sys.env_buf, env_transfer_size);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_command_buf);

    SDL_ReleaseGPUTransferBuffer(gpu_device, per_instance_transfer_buf);
    SDL_ReleaseGPUTransferBuffer(gpu_device, per_vertex_transfer_buf);
    SDL_ReleaseGPUTransferBuffer(gpu_device, transform_transfer_buf);
    SDL_ReleaseGPUTransferBuffer(gpu_device, color_transfer_buf);
    SDL_ReleaseGPUTransferBuffer(gpu_device, view_transfer_buf);
    SDL_ReleaseGPUTransferBuffer(gpu_device, env_transfer_buf);

    if (graphics_sys.graphics_object_pool.len <= 0) {
        return true;
    }

    // draw command
    SDL_GPUCommandBuffer* command_buf = SDL_AcquireGPUCommandBuffer(gpu_device); {

        SDL_GPUTexture* swapchain_texture;
        u32 swapchain_width, swapchain_height;
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buf, sdl_window,
                                              &swapchain_texture, &swapchain_width, &swapchain_height);
        if (swapchain_texture == nullptr) {
            SDL_SubmitGPUCommandBuffer(command_buf);
            return false;
        }

        SDL_GPURenderPass *render_pass; {
            SDL_GPUColorTargetInfo color_target_info = {
                .texture = swapchain_texture,
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE
            };
            SDL_GPUDepthStencilTargetInfo depth_target_info = {
                .texture = graphics_sys.depth_texture,
                .clear_depth = 1,
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
            };
            render_pass = SDL_BeginGPURenderPass(command_buf, &color_target_info, 1, &depth_target_info);

            // bind pipeline
            SDL_BindGPUGraphicsPipeline(render_pass, graphics_sys.pipeline);

            // vertex storage
            SDL_GPUBuffer* vert_storage_buffers[] = {
                graphics_sys.per_instance_accessor_buf,
                graphics_sys.per_vertex_accessor_buf,
                graphics_sys.model_header_buf,
                graphics_sys.vertices_buf,
                graphics_sys.normals_buf,
                graphics_sys.indices_buf,
                graphics_sys.transform_buf,
                graphics_sys.view_buf
            };
            SDL_BindGPUVertexStorageBuffers(render_pass, 0, vert_storage_buffers, GRAPHICS_VERTEX_SHADER_BUF_NUM);

            // fragment storage
            SDL_GPUBuffer* frag_storage_buffers[] = {
                graphics_sys.per_instance_accessor_buf,
                graphics_sys.per_vertex_accessor_buf,
                graphics_sys.color_buf,
                graphics_sys.env_buf,
                graphics_sys.view_buf,
            };
            SDL_BindGPUFragmentStorageBuffers(render_pass, 0, frag_storage_buffers, GRAPHICS_FRAGMENT_SHADER_BUF_NUM);

            // draw primitives
            for (i32 i = 0; i < graphics_sys.draw_call_data_len; i++) {
                GraphicsDrawCallData draw_call_data = graphics_sys.draw_call_data_buf[i];
                GraphicsModelData model_data = graphics_sys.model_data_list_buf[draw_call_data.model_idx];

                SDL_DrawGPUPrimitives(render_pass, model_data.indices_num, 1, 0, i);
            }

        } SDL_EndGPURenderPass(render_pass);

    } SDL_SubmitGPUCommandBuffer(command_buf);

    graphics_sys.draw_call_data_len = 0;

    return true;
}

GraphicsObject *graphics_create_object(GraphicsObject *object = nullptr) {
    return pool_create(&graphics_sys.graphics_object_pool, object);
}

bool graphics_destroy_object(GraphicsObject *object) {
    return pool_destroy(&graphics_sys.graphics_object_pool, object);
}

void graphics_handle_upload_to_buf(SDL_GPUBuffer *gpu_buf, i32 offset, void *data_buf, i32 data_buf_size) {
    if (data_buf_size <= 0 || data_buf == nullptr) return;

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
                .offset = (u32)offset,
                .size = (u32)data_buf_size
            };
            SDL_UploadToGPUBuffer(copy_pass, &transfer_buf_location, &buf_region, false);
        } SDL_EndGPUCopyPass(copy_pass);
    } SDL_SubmitGPUCommandBuffer(upload_command_buf);

    SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buf);
}

i32 graphics_load_model(const char *path) {
    char true_path[256];
    strcpy(true_path, path);
    file_to_asset_path(true_path, sizeof(true_path));

    cgltf_options load_model_options = {};
    cgltf_data *model_data = nullptr;

    if (cgltf_parse_file(&load_model_options, true_path, &model_data) != cgltf_result_success) {
        printf("failed to parse model: path = [%s]\n", true_path);
        return -1;
    }
    if (cgltf_load_buffers(&load_model_options, model_data, true_path) != cgltf_result_success) {
        printf("failed to load model buffer: path = [%s]\n", true_path);
        cgltf_free(model_data);
        return -1;
    }

    // calculate total size
    i32 total_vertices_num = 0;
    i32 total_normals_num = 0;
    i32 total_indices_num = 0;

    for (i32 i = 0; i < model_data->meshes_count; i++) {
        cgltf_mesh *mesh = &model_data->meshes[i];
        for (i32 j = 0; j < mesh->primitives_count; j++) {
            cgltf_primitive *primitive = &mesh->primitives[j];

            for (cgltf_size k = 0; k < primitive->attributes_count; k++) {
                if (primitive->attributes[k].type == cgltf_attribute_type_position) {
                    total_vertices_num += (i32)primitive->attributes[k].data->count;
                }
                if (primitive->attributes[k].type == cgltf_attribute_type_normal) {
                    total_normals_num += (i32)primitive->attributes[k].data->count;
                }
            }

            if (primitive->indices != nullptr) {
                total_indices_num += (i32)primitive->indices->count;
            }
        }
    }

    i32 vertices_data_buf_size = 4 * total_vertices_num * sizeof(f32);
    i32 normals_data_buf_size  = 4 * total_normals_num * sizeof(f32);
    i32 indices_data_buf_size  = 4 * total_indices_num * sizeof(i32);

    f32 *vertices_data_buf = (f32*)malloc(vertices_data_buf_size);
    f32 *normals_data_buf  = (f32*)malloc(normals_data_buf_size);
    i32 *indices_data_buf  = (i32*)malloc(indices_data_buf_size);

    i32 vertices_idx_offset = 0;
    i32 normals_idx_offset = 0;
    i32 indices_idx_offset  = 0;
    i32 base_vert_offset = 0;

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

            if (vertices_accessor) {
                u32 vertices_num = vertices_accessor->count;
                for (u32 v = 0; v < vertices_num; v++) {
                    f32 pos[3] = {0.0f, 0.0f, 0.0f};
                    cgltf_accessor_read_float(vertices_accessor, v, pos, 3);
                    vertices_data_buf[vertices_idx_offset + 0] = pos[0];
                    vertices_data_buf[vertices_idx_offset + 1] = pos[1];
                    vertices_data_buf[vertices_idx_offset + 2] = pos[2];
                    vertices_idx_offset += 4;
                }
            }

            // normals
            cgltf_accessor *normals_accessor = nullptr;
            for (cgltf_size k = 0; k < primitive->attributes_count; k++) {
                if (primitive->attributes[k].type == cgltf_attribute_type_normal) {
                    normals_accessor = primitive->attributes[k].data;
                    break;
                }
            }

            if (normals_accessor) {
                u32 normals_num = normals_accessor->count;
                for (u32 n = 0; n < normals_num; n++) {
                    f32 norm[3] = {0.0f, 0.0f, 0.0f};
                    cgltf_accessor_read_float(normals_accessor, n, norm, 3);
                    normals_data_buf[normals_idx_offset + 0] = norm[0];
                    normals_data_buf[normals_idx_offset + 1] = norm[1];
                    normals_data_buf[normals_idx_offset + 2] = norm[2];
                    normals_idx_offset += 4;
                }
            }

            // indices
            cgltf_accessor *indices_accessor = primitive->indices;
            if (indices_accessor) {
                u32 indices_num = indices_accessor->count;
                cgltf_accessor_unpack_indices(indices_accessor, &indices_data_buf[indices_idx_offset], sizeof(i32), indices_num);

                for (u32 idx_i = 0; idx_i < indices_num; idx_i++) {
                    cgltf_size raw_idx = cgltf_accessor_read_index(indices_accessor, idx_i);

                    indices_data_buf[indices_idx_offset] = raw_idx + base_vert_offset;

                    indices_idx_offset += 4;
                }

                base_vert_offset += (vertices_accessor ? vertices_accessor->count : 0);
            }
        }
    }

    cgltf_free(model_data);

    i32 model_idx = graphics_sys.model_data_list_len;

    // model header
    GraphicsModelHeader model_header = {
        .vertices_offset = (u32)(graphics_sys.vertices_buf_offset / 16),
        .normals_offset  = (u32)(graphics_sys.normals_buf_offset / 16),
        .indices_offset  = (u32)(graphics_sys.indices_buf_offset / 16)
    };

    graphics_handle_upload_to_buf(graphics_sys.model_header_buf, model_idx * sizeof(GraphicsModelHeader), &model_header, sizeof(GraphicsModelHeader));
    graphics_handle_upload_to_buf(graphics_sys.vertices_buf, graphics_sys.vertices_buf_offset, vertices_data_buf, vertices_data_buf_size);
    graphics_handle_upload_to_buf(graphics_sys.normals_buf, graphics_sys.normals_buf_offset, normals_data_buf, normals_data_buf_size);
    graphics_handle_upload_to_buf(graphics_sys.indices_buf, graphics_sys.indices_buf_offset, indices_data_buf, indices_data_buf_size);

    free(vertices_data_buf);
    free(normals_data_buf);
    free(indices_data_buf);

    graphics_sys.vertices_buf_offset += vertices_data_buf_size;
    graphics_sys.normals_buf_offset  += normals_data_buf_size;
    graphics_sys.indices_buf_offset  += indices_data_buf_size;

    // model data
    graphics_sys.model_data_list_buf[model_idx] = (GraphicsModelData) {
        .indices_num = total_indices_num
    };
    graphics_sys.model_data_list_len++;

    return model_idx;
}

}
