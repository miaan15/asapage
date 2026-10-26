// graphics system
// using bindless appoarch, so that there're mostly just some static storage buffer that any shader can read on
// the shader will use gl_InstanceIndex or gl_VertexIndex to reference the actual render data
//
// there'll be called 'Accessor' which should store the index that can be use on actual data buffer
// e.g: PerInstanceAccessor::model_idx: per_instance_accessor_buf[gl_InstanceIndex].model_idx to take out the model accessor idx

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
import pool;

export import graphics_shader;
export import graphics_texture;

export namespace asapage {

// all these structs are supposed to be direct map to GPU RAM struct (glsl std430)

struct GraphicsGPUPerInstanceAccessor {
// uint model_accessor_idx;
// uint object_idx;
    u32 model_accessor_idx;
    u32 object_idx;
};

struct GraphicsGPUPerVertexAccessor {
// uint placeholder;
};

struct GraphicsGPUModelAccessor {
// uint vertex_offset;
// uint index_offset;
    u32 vertex_offset;
    u32 index_offset;
};

struct GraphicsGPUModelVertex {
// vec3 pos;
// vec3 normal;
// vec2 uv;
    alignas(16) Vec3 pos;
    alignas(16) Vec3 normal;
    alignas(16) Vec2 uv;
};

// suppose there is GraphicsModelIndex struct, but it is a 'uint' in shader

// suppose there is GraphicsTransform struct, but it is a 'mat4' in shader

struct GraphicsGPUObjectData {
// vec3 color;
    alignas(16) Vec3 color;
};

struct GraphicsGPUView {
// vec2 view_size;
// vec2 view_range;
// vec3 view_eye;
// vec3 view_center;
// vec3 view_up;
// float view_fov;
    alignas(16) Vec2 size;
    Vec2 range;
    alignas(16) Vec3 eye;
    alignas(16) Vec3 center;
    alignas(16) Vec3 up;
    f32 fov;
};

struct GraphicsGPUEnv {
// vec3 env_light_pos;
// float env_light_intensity;
    alignas(16) Vec3 light_pos;
    f32 light_intensity;
};

// number of storage buffer in each shader
constexpr i32 GRAPHICS_VERTEX_SHADER_BUF_NUM = 8;
constexpr i32 GRAPHICS_FRAGMENT_SHADER_SAMPLER_NUM = 0;
constexpr i32 GRAPHICS_FRAGMENT_SHADER_BUF_NUM = 2;

// FIXME idk, maybe move these to some global config
constexpr i32 GRAPHICS_MAX_OBJECT_NUM = 1e5; // 100k
constexpr i32 GRAPHICS_MAX_MODEL_NUM = 100;

constexpr i32 GRAPHICS_VERTICES_BUF_SIZE = 500 << 20; // 500MB
constexpr i32 GRAPHICS_INDICES_BUF_SIZE = 200 << 20; // 200MB

// normal CPU structs

// graphics object is a 3D instance in game (with shape/model and transform and some more data about render, graphics)
struct GraphicsObject {
    i32 idx;

    i32 model_idx;

    Vec3 position;
    Quat rotation;
    Vec3 scale;

    Vec3 color;
};

// meta data of a 3D model, since actual 3D model data should be in GPU RAM, so meta data are just to access/handle it
struct GraphicsModelMeta {
    i32 idx;

    const char *model_path;

    i32 vertices_num;
    i32 indices_num;
};

// data of how to draw in a frame
struct GraphicsDrawCall {
    i32 indices_num;
};

struct GraphicsSys {
    // config
    SDL_GPUTextureFormat screen_format;

    GraphicsGPUView view;
    GraphicsGPUEnv env;

    // pipeline stuff
    SDL_GPUTextureFormat depth_format;
    SDL_GPUTexture *depth_texture;

    SDL_GPUGraphicsPipeline *pipeline;

    // GPU buffers
    SDL_GPUBuffer *per_instance_accessor_buf;
    u32 per_instance_accessor_buf_cap;

    SDL_GPUBuffer *per_vertex_accessor_buf;
    u32 per_vertex_accessor_buf_cap;

    SDL_GPUBuffer *model_accessor_buf;
    u32 model_accessor_buf_cap;

    SDL_GPUBuffer *model_vertex_buf;
    u32 model_vertex_buf_cap;

    SDL_GPUBuffer *model_index_buf;
    u32 model_index_buf_cap;

    SDL_GPUBuffer *transform_buf;
    u32 transform_buf_cap;

    SDL_GPUBuffer *object_data_buf;
    u32 object_data_buf_cap;

    SDL_GPUBuffer *view_buf;
    u32 view_buf_cap;

    SDL_GPUBuffer *env_buf;
    u32 env_buf_cap;

    // in game
    Pool<GraphicsObject> graphics_object_pool;

    i32 model_meta_list_len;
    GraphicsModelMeta *model_meta_list;

    //
    i32 total_loaded_model_vertex_num;
    i32 total_loaded_model_index_num;
};
inline GraphicsSys graphics_sys = {};

void graphics_sys_init() {
    graphics_sys.screen_format = SDL_GetGPUSwapchainTextureFormat(gpu_device, sdl_window);
    graphics_sys.depth_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

    { // depth_texture
        SDL_GPUTextureCreateInfo texture_cinfo = {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .format = graphics_sys.depth_format,
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
        SDL_GPUShader *vertex_shader = graphics_shader_load("default_shader.vert", false, SHADER_STAGE::VERTEX,
                                                            0,
                                                            0,
                                                            GRAPHICS_VERTEX_SHADER_BUF_NUM,
                                                            0);
        SDL_GPUShader *fragment_shader = graphics_shader_load("default_shader.frag", false, SHADER_STAGE::FRAGMENT,
                                                              GRAPHICS_FRAGMENT_SHADER_SAMPLER_NUM,
                                                              0,
                                                              GRAPHICS_FRAGMENT_SHADER_BUF_NUM,
                                                              0);

        // create info
        SDL_GPUGraphicsPipelineCreateInfo pipeline_cinfo = {};
        pipeline_cinfo.vertex_shader = vertex_shader;
        pipeline_cinfo.fragment_shader = fragment_shader;
        pipeline_cinfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // - pipeline target info
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

        // - rasterizer
        SDL_GPURasterizerState rasterizer_state = {
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
        };
        pipeline_cinfo.rasterizer_state = rasterizer_state;

        // - depth stencil
        SDL_GPUDepthStencilState depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = true,
            .enable_depth_write = true
        };
        pipeline_cinfo.depth_stencil_state = depth_stencil_state;

        //
        graphics_sys.pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &pipeline_cinfo);

        SDL_ReleaseGPUShader(gpu_device, vertex_shader);
        SDL_ReleaseGPUShader(gpu_device, fragment_shader);
    }

    // buffers
    {
        SDL_GPUBufferCreateInfo buf_cinfo = { .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ };

        graphics_sys.per_instance_accessor_buf_cap = buf_cinfo.size =
            GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsGPUPerInstanceAccessor);
        graphics_sys.per_instance_accessor_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);

        graphics_sys.per_vertex_accessor_buf_cap = buf_cinfo.size =
            3 * GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsGPUPerVertexAccessor);
        graphics_sys.per_vertex_accessor_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);

        graphics_sys.model_accessor_buf_cap = buf_cinfo.size =
            GRAPHICS_MAX_MODEL_NUM * sizeof(GraphicsGPUModelAccessor);
        graphics_sys.model_accessor_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);

        graphics_sys.model_vertex_buf_cap = buf_cinfo.size =
            GRAPHICS_VERTICES_BUF_SIZE;
        graphics_sys.model_vertex_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);

        graphics_sys.model_index_buf_cap = buf_cinfo.size =
            GRAPHICS_INDICES_BUF_SIZE;
        graphics_sys.model_index_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);

        graphics_sys.transform_buf_cap = buf_cinfo.size =
            GRAPHICS_MAX_OBJECT_NUM * 4 * 4 * sizeof(f32);
        graphics_sys.transform_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);

        graphics_sys.object_data_buf_cap = buf_cinfo.size =
            GRAPHICS_MAX_OBJECT_NUM * sizeof(GraphicsGPUObjectData);
        graphics_sys.object_data_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);

        graphics_sys.view_buf_cap = buf_cinfo.size =
            sizeof(GraphicsGPUView);
        graphics_sys.view_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);

        graphics_sys.env_buf_cap = buf_cinfo.size =
            sizeof(GraphicsGPUEnv);
        graphics_sys.env_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);
    }

    //
    pool_init(&graphics_sys.graphics_object_pool,
              omni_arena_ptr, GRAPHICS_MAX_OBJECT_NUM, offsetof(GraphicsObject, idx));

    graphics_sys.model_meta_list_len = 0;
    graphics_sys.model_meta_list =
        (GraphicsModelMeta*)arena_alloc(omni_arena_ptr, GRAPHICS_MAX_MODEL_NUM * sizeof(GraphicsModelMeta));
}

void graphics_sys_destroy() {
    SDL_ReleaseGPUTexture(gpu_device, graphics_sys.depth_texture);
    SDL_ReleaseGPUGraphicsPipeline(gpu_device, graphics_sys.pipeline);

    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.per_instance_accessor_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.per_vertex_accessor_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.model_accessor_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.model_vertex_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.model_index_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.transform_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.object_data_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.view_buf);
    SDL_ReleaseGPUBuffer(gpu_device, graphics_sys.env_buf);
}

bool graphics_draw() {
    // this is called every frame, should be a hot af

    i32 draw_call_num = 0;
    auto *draw_call_buf =
        (GraphicsDrawCall*)arena_alloc(frame_arena_ptr, graphics_sys.graphics_object_pool.len * sizeof(GraphicsDrawCall));

    // updata/upload all object data, global data to GPU
    {
        auto *command_buf = SDL_AcquireGPUCommandBuffer(gpu_device);
        auto *copy_pass = SDL_BeginGPUCopyPass(command_buf);

        // use only 1 transfer buffer to upload every buffer to GPU
        // offset of every buffer inside the trasnfer buffer
        i32 offset = 0;

        i32 per_instance_accessor_transfer_offset = offset;
        offset = mem_align_up(offset + graphics_sys.per_instance_accessor_buf_cap, 16);

        i32 per_vertex_accessor_transfer_offset = offset;
        offset = mem_align_up(offset + graphics_sys.per_vertex_accessor_buf_cap, 16);

        i32 transform_transfer_offset = offset;
        offset = mem_align_up(offset + graphics_sys.transform_buf_cap, 16);

        i32 object_data_transfer_offset = offset;
        offset = mem_align_up(offset + graphics_sys.object_data_buf_cap, 16);

        i32 view_transfer_offset = offset;
        offset = mem_align_up(offset + graphics_sys.view_buf_cap, 16);

        i32 env_transfer_offset = offset;
        offset = mem_align_up(offset + graphics_sys.env_buf_cap, 16);

        // create transfer buf
        SDL_GPUTransferBufferCreateInfo transfer_buf_cinfo = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = (u32)offset
        };
        auto *transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);

        char *transfer_map_ptr = (char*)SDL_MapGPUTransferBuffer(gpu_device, transfer_buf, true);

        auto *per_instance_map_ptr =
            (GraphicsGPUPerInstanceAccessor*)(transfer_map_ptr + per_instance_accessor_transfer_offset);
        auto *per_vertex_map_ptr =
            (GraphicsGPUPerVertexAccessor*)(transfer_map_ptr + per_vertex_accessor_transfer_offset);
        auto *transform_map_ptr = (Mat4*)(transfer_map_ptr + transform_transfer_offset);
        auto *object_data_map_ptr = (GraphicsGPUObjectData*)(transfer_map_ptr + object_data_transfer_offset);
        auto *view_map_ptr = (GraphicsGPUView*)(transfer_map_ptr + view_transfer_offset);
        auto *env_map_ptr = (GraphicsGPUEnv*)(transfer_map_ptr + env_transfer_offset);

        // map data to transfer buf
        // because this func need high performance, bypass the pool api call, use the raw data inside
        for (i32 object_idx = 1; object_idx < graphics_sys.graphics_object_pool.max_idx; object_idx++) {
            GraphicsObject object = graphics_sys.graphics_object_pool.data_buf[object_idx];
            if (object.idx == 0) { continue; } // if dead

            // FIXME TODO use CPU for now, but this should be in GPU
            Mat4 transform_matrix = mat4_translation(object.position) *
                                    mat4_rotation(object.rotation) *
                                    mat4_scale(object.scale);
            transform_map_ptr[object_idx] = transform_matrix;

            GraphicsGPUObjectData object_data = {
                .color = object.color
            };
            object_data_map_ptr[object_idx] = object_data;

            GraphicsGPUPerInstanceAccessor per_instance = {
                .model_accessor_idx = (u32)object.model_idx,
                .object_idx = (u32)object_idx,
            };
            per_instance_map_ptr[draw_call_num] = per_instance;

            // TODO do this latter
            // for (i32 i = 0; i < 3; i++) {
            //     GraphicsGPUPerVertexAccessor per_vertex
            //     per_vertex_map_ptr[draw_call_num]
            // }

            draw_call_buf[draw_call_num] = {
                .indices_num = graphics_sys.model_meta_list[object.model_idx].indices_num
            };

            draw_call_num++;
        }

        *view_map_ptr = graphics_sys.view;
        *env_map_ptr = graphics_sys.env;

        //
        SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buf);

        SDL_GPUTransferBufferLocation src_location = { .transfer_buffer = transfer_buf, };
        SDL_GPUBufferRegion dest_region = { .offset = 0, };

        // actual upload all data to GPU
        src_location.offset = per_instance_accessor_transfer_offset;
        dest_region.buffer = graphics_sys.per_instance_accessor_buf;
        dest_region.size = graphics_sys.per_instance_accessor_buf_cap;
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dest_region, true);

        src_location.offset = per_vertex_accessor_transfer_offset;
        dest_region.buffer = graphics_sys.per_vertex_accessor_buf;
        dest_region.size = graphics_sys.per_vertex_accessor_buf_cap;
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dest_region, true);

        src_location.offset = transform_transfer_offset;
        dest_region.buffer = graphics_sys.transform_buf;
        dest_region.size = graphics_sys.transform_buf_cap;
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dest_region, true);

        src_location.offset = object_data_transfer_offset;
        dest_region.buffer = graphics_sys.object_data_buf;
        dest_region.size = graphics_sys.object_data_buf_cap;
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dest_region, true);

        src_location.offset = view_transfer_offset;
        dest_region.buffer = graphics_sys.view_buf;
        dest_region.size = graphics_sys.view_buf_cap;
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dest_region, true);

        src_location.offset = env_transfer_offset;
        dest_region.buffer = graphics_sys.env_buf;
        dest_region.size = graphics_sys.env_buf_cap;
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dest_region, true);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(command_buf);

        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buf);
    }

    // draw command
    auto* command_buf = SDL_AcquireGPUCommandBuffer(gpu_device); {

        SDL_GPUTexture* swapchain_texture;
        u32 swapchain_width, swapchain_height;
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buf, sdl_window,
                                              &swapchain_texture, &swapchain_width, &swapchain_height);
        if (swapchain_texture == nullptr) {
            SDL_SubmitGPUCommandBuffer(command_buf);
            return false;
        }

        // render pass
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
        auto *render_pass = SDL_BeginGPURenderPass(command_buf, &color_target_info, 1, &depth_target_info); {

            // bind pipeline
            SDL_BindGPUGraphicsPipeline(render_pass, graphics_sys.pipeline);

            // vertex storage
            SDL_GPUBuffer *vert_storage_buffers[] = {
                graphics_sys.per_instance_accessor_buf,
                graphics_sys.per_vertex_accessor_buf,
                graphics_sys.model_accessor_buf,
                graphics_sys.model_vertex_buf,
                graphics_sys.model_index_buf,
                graphics_sys.transform_buf,
                graphics_sys.object_data_buf,
                graphics_sys.view_buf
            };
            SDL_BindGPUVertexStorageBuffers(render_pass,
                                            0, vert_storage_buffers, GRAPHICS_VERTEX_SHADER_BUF_NUM);

            // fragment storage
            SDL_GPUBuffer *frag_storage_buffers[] = {
                graphics_sys.env_buf,
                graphics_sys.view_buf,
            };
            SDL_BindGPUFragmentStorageBuffers(render_pass,
                                              0, frag_storage_buffers, GRAPHICS_FRAGMENT_SHADER_BUF_NUM);

            // draw primitives
            for (i32 i = 0; i < draw_call_num; i++) {
                auto draw_call = draw_call_buf[i];

                SDL_DrawGPUPrimitives(render_pass, draw_call.indices_num, 1, 0, i);
            }

        } SDL_EndGPURenderPass(render_pass);

    } SDL_SubmitGPUCommandBuffer(command_buf);

    return true;
}

GraphicsObject *graphics_create_object(GraphicsObject *object = nullptr) {
    auto *res = pool_create(&graphics_sys.graphics_object_pool, object);
    res->rotation = { 0, 0, 0, 1 };
    res->scale = { 1, 1, 1 };
    return res;
}

bool graphics_destroy_object(GraphicsObject *object) {
    return pool_destroy(&graphics_sys.graphics_object_pool, object);
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

    // calculated malloc size first
    i32 total_vertex_num = 0;
    i32 total_index_num = 0;

    for (i32 i = 0; i < model_data->meshes_count; i++) {
        cgltf_mesh *mesh = &model_data->meshes[i];
        for (i32 j = 0; j < mesh->primitives_count; j++) {
            cgltf_primitive *primitive = &mesh->primitives[j];

            for (i32 k = 0; k < primitive->attributes_count; k++) {
                if (primitive->attributes[k].type == cgltf_attribute_type_position) {
                    total_vertex_num += primitive->attributes[k].data->count;
                    break;
                }
            }

            if (primitive->indices != nullptr) {
                total_index_num += primitive->indices->count;
            }
        }
    }

    auto *vertex_pos_data_buf = (f32*)malloc(4 * total_vertex_num * sizeof(f32));
    auto *vertex_normal_data_buf = (f32*)malloc(4 * total_vertex_num * sizeof(f32));
    auto *vertex_uv_data_buf = (f32*)malloc(4 * total_vertex_num * sizeof(f32));
    auto *index_data_buf = (i32*)malloc(total_index_num * sizeof(u32));

    i32 cur_vertex_data_buf_offset = 0;
    i32 cur_index_data_buf_offset = 0;

    i32 cur_vertex_idx_offset = 0;

    for (i32 i = 0; i < model_data->meshes_count; i++) {
        cgltf_mesh *mesh = &model_data->meshes[i];
        for (i32 j = 0; j < mesh->primitives_count; j++) {
            cgltf_primitive *primitive = &mesh->primitives[j];

            cgltf_accessor *pos_accessor = nullptr;
            cgltf_accessor *normal_accessor = nullptr;
            cgltf_accessor *uv_accessor = nullptr;
            cgltf_accessor *index_accessor = primitive->indices;

            for (i32 k = 0; k < primitive->attributes_count; k++) {
                cgltf_attribute *attribute = &primitive->attributes[k];
                switch (attribute->type) {
                case cgltf_attribute_type_position: pos_accessor = attribute->data; break;
                case cgltf_attribute_type_normal:   normal_accessor = attribute->data; break;
                case cgltf_attribute_type_texcoord: uv_accessor = attribute->data; break;
                default: break;
                }
            }

            if (!pos_accessor) continue;

            i32 vertex_num = pos_accessor->count;

            for (i32 v = 0; v < vertex_num; v++) {
                f32 temp_pos[3] = { 0, 0, 0 };
                cgltf_accessor_read_float(pos_accessor, v, temp_pos, 3);
                vertex_pos_data_buf[cur_vertex_data_buf_offset + v * 4 + 0] = temp_pos[0];
                vertex_pos_data_buf[cur_vertex_data_buf_offset + v * 4 + 1] = temp_pos[1];
                vertex_pos_data_buf[cur_vertex_data_buf_offset + v * 4 + 2] = temp_pos[2];
                vertex_pos_data_buf[cur_vertex_data_buf_offset + v * 4 + 3] = 1;

                f32 temp_normal[3] = { 0, 1, 0 };
                if (normal_accessor) { cgltf_accessor_read_float(normal_accessor, v, temp_normal, 3); }
                vertex_normal_data_buf[cur_vertex_data_buf_offset + v * 4 + 0] = temp_normal[0];
                vertex_normal_data_buf[cur_vertex_data_buf_offset + v * 4 + 1] = temp_normal[1];
                vertex_normal_data_buf[cur_vertex_data_buf_offset + v * 4 + 2] = temp_normal[2];
                vertex_normal_data_buf[cur_vertex_data_buf_offset + v * 4 + 3] = 0;

                f32 temp_uv[2] = { 0, 0 };
                if (uv_accessor) { cgltf_accessor_read_float(uv_accessor, v, temp_uv, 2); }
                vertex_uv_data_buf[cur_vertex_data_buf_offset + v * 4 + 0] = temp_uv[0];
                vertex_uv_data_buf[cur_vertex_data_buf_offset + v * 4 + 1] = temp_uv[1];
                vertex_uv_data_buf[cur_vertex_data_buf_offset + v * 4 + 2] = 0;
                vertex_uv_data_buf[cur_vertex_data_buf_offset + v * 4 + 3] = 0;
            }

            i32 indices_num = index_accessor ? (i32)index_accessor->count : 0;
            if (index_accessor != nullptr) {
                cgltf_accessor_unpack_indices(index_accessor,
                                              &index_data_buf[cur_index_data_buf_offset], sizeof(i32), indices_num);
                for (i32 k = 0; k < indices_num; k++) {
                    index_data_buf[cur_index_data_buf_offset + k] += cur_vertex_idx_offset;
                }
            }

            cur_vertex_data_buf_offset += 4 * vertex_num;
            cur_index_data_buf_offset += indices_num;

            cur_vertex_idx_offset += vertex_num;
        }
    }
    cgltf_free(model_data);

    i32 model_idx = graphics_sys.model_meta_list_len;

    // upload to GPU
    {
        i32 accessor_transfer_size = sizeof(GraphicsGPUModelAccessor);
        i32 vertices_transfer_size = total_vertex_num * sizeof(GraphicsGPUModelVertex);
        i32 indices_transfer_size = total_index_num * sizeof(u32);

        i32 accessor_transfer_offset = 0;
        i32 vertices_transfer_offset = mem_align_up(accessor_transfer_offset + accessor_transfer_size, 16);
        i32 indices_transfer_offset  = mem_align_up(vertices_transfer_offset + vertices_transfer_size, 16);

        i32 total_transfer_size = indices_transfer_offset + indices_transfer_size;

        SDL_GPUTransferBufferCreateInfo transfer_cinfo = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = (u32)total_transfer_size
        };
        auto *transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_cinfo);

        char *transfer_map_ptr = (char*)SDL_MapGPUTransferBuffer(gpu_device, transfer_buf, false);

        auto *model_accessor_map_ptr = (GraphicsGPUModelAccessor*)(transfer_map_ptr + accessor_transfer_offset);
        model_accessor_map_ptr->vertex_offset = (u32)graphics_sys.total_loaded_model_vertex_num;
        model_accessor_map_ptr->index_offset = (u32)graphics_sys.total_loaded_model_index_num;

        auto *model_vertex_map_ptr = (GraphicsGPUModelVertex*)(transfer_map_ptr + vertices_transfer_offset);
        for (i32 v = 0; v < total_vertex_num; v++) {
            model_vertex_map_ptr[v].pos = {
                vertex_pos_data_buf[v * 4 + 0],
                vertex_pos_data_buf[v * 4 + 1],
                vertex_pos_data_buf[v * 4 + 2]
            };
            model_vertex_map_ptr[v].normal = {
                vertex_normal_data_buf[v * 4 + 0],
                vertex_normal_data_buf[v * 4 + 1],
                vertex_normal_data_buf[v * 4 + 2]
            };
            model_vertex_map_ptr[v].uv = {
                vertex_uv_data_buf[v * 4 + 0],
                vertex_uv_data_buf[v * 4 + 1]
            };
        }

        auto *model_index_map_ptr = (u32*)(transfer_map_ptr + indices_transfer_offset);
        memcpy(model_index_map_ptr, index_data_buf, indices_transfer_size);

        SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buf);

        // upload command
        auto *command_buf = SDL_AcquireGPUCommandBuffer(gpu_device);
        auto *copy_pass = SDL_BeginGPUCopyPass(command_buf);

        SDL_GPUTransferBufferLocation src_location = { .transfer_buffer = transfer_buf };
        SDL_GPUBufferRegion dest_region = {};

        src_location.offset = (u32)accessor_transfer_offset;
        dest_region.buffer = graphics_sys.model_accessor_buf;
        dest_region.offset = (u32)(model_idx * sizeof(GraphicsGPUModelAccessor));
        dest_region.size = (u32)accessor_transfer_size;
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dest_region, false);

        src_location.offset = (u32)vertices_transfer_offset;
        dest_region.buffer = graphics_sys.model_vertex_buf;
        dest_region.offset = (u32)(graphics_sys.total_loaded_model_vertex_num * sizeof(GraphicsGPUModelVertex));
        dest_region.size = (u32)vertices_transfer_size;
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dest_region, false);

        src_location.offset = (u32)indices_transfer_offset;
        dest_region.buffer = graphics_sys.model_index_buf;
        dest_region.offset = (u32)(graphics_sys.total_loaded_model_index_num * sizeof(u32));
        dest_region.size = (u32)indices_transfer_size;
        SDL_UploadToGPUBuffer(copy_pass, &src_location, &dest_region, false);

        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(command_buf);

        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buf);

        //
        graphics_sys.total_loaded_model_vertex_num += total_vertex_num;
        graphics_sys.total_loaded_model_index_num += total_index_num;
    }

    free(vertex_pos_data_buf);
    free(vertex_normal_data_buf);
    free(vertex_uv_data_buf);
    free(index_data_buf);

    graphics_sys.model_meta_list[model_idx] = {
        .idx = model_idx,
        .model_path = path,
        .vertices_num = total_vertex_num,
        .indices_num = total_index_num,
    };
    graphics_sys.model_meta_list_len++;

    printf("loaded model [%d]: path = [%s]; vertex_num = [%d]; triangle_num = [%d]\n", model_idx, true_path, total_vertex_num, total_index_num / 3);

    return model_idx;
}

}
