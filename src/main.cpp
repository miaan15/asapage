#include <cassert>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

import define;
import mem;
import internal;
import context;
import file;
import input;
import shader;

import math;
import pool;
import map;
import mapstr;

import graphics_model;

using namespace asapage;

struct alignas(16) ViewData {
    f32 view_size[2];
    f32 __padding_0[2];
    f32 view_eye[3];
    f32 __padding_1;
    f32 view_center[3];
    f32 __padding_2;
    f32 view_up[3];
    f32 __padding_3;
    f32 view_range[2];
    f32 view_fov;
    f32 __padding_4;
};

struct alignas(16) EnvData {
    f32 env_light_pos[3];
    f32 env_light_intensity;
};

struct alignas(16) Transform {
    f32 position[3];
    f32 __padding_0;
    f32 rotation[4];
    f32 scale[3];
    f32 __padding_1;
};

struct alignas(16) InstanceAccessor {
    u32 model_idx;
    u32 transform_idx;
};

struct alignas(16) VertexAccessor {
    u32 placeholder;
};

struct alignas(16) ModelHeader {
    u32 vertices_offset;
    u32 normals_offset;
    u32 indices_offset;
    u32 __padding_0;
};

int main() {
    internal_init();

    file_sys_init();

    input_sys_init();

    // FIXME generalize this
    int window_w, window_h;
    SDL_GetWindowSizeInPixels(window, &window_w, &window_h);

    // uniform
    ViewData view_data = {
        .view_size = { (f32)window_w, (f32)window_h },
        .view_eye = { 0, 5, 7 },
        .view_center = { 0, 0, 0 },
        .view_up = { 0, 1, 0 },
        .view_range = { 0.1f, 100.0f },
        .view_fov = 20.0f * 3.14159f / 180.0f
    };

    EnvData env_data = {
        .env_light_pos = { 0, 5, 7 },
        .env_light_intensity = 1
    };

    f32 material_color[4] = { 0.8f, 0.3f, 0.2f, 1.0f };

    InstanceAccessor instance_accessors[1] = {
        { .model_idx = 0, .transform_idx = 0 }
    };

    VertexAccessor vertex_accessors[1] = {
        { .placeholder = 0 }
    };

    ModelHeader model_headers[1] = {
        { .vertices_offset = 0, .normals_offset = 0, .indices_offset = 0 }
    };

    Transform transform_list[1] = {
        {
            .position = { 0, 0, 0 },
            .rotation = { 0, 0, 0, 1 },
            .scale = { 1, 1, 1 },
        }
    };

    // shader + pipeline
    char vertex_shader_path[512] = "mvp_shader.vert";
    file_to_asset_path(vertex_shader_path, 512);
    char fragment_shader_path[512] = "mvp_shader.frag";
    file_to_asset_path(fragment_shader_path, 512);

    SDL_GPUShader *vertex_shader = create_shader_glsl_file(vertex_shader_path, SHADER_STAGE::VERTEX, 0, 0, 8, 0);
    SDL_GPUShader *fragment_shader = create_shader_glsl_file(fragment_shader_path, SHADER_STAGE::FRAGMENT, 0, 0, 3, 0);
    if (vertex_shader == nullptr || fragment_shader == nullptr) { assert(false); }

    SDL_GPUGraphicsPipeline *pipeline;
    init_pipeline(&pipeline, vertex_shader, fragment_shader);

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    SDL_GPUBufferCreateInfo storage_buf_cinfo = {
        .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
        .size = 1 << 20 // 1 MB
    };
    SDL_GPUBuffer *vertices_buf = SDL_CreateGPUBuffer(gpu_device, &storage_buf_cinfo);
    SDL_GPUBuffer *indices_buf  = SDL_CreateGPUBuffer(gpu_device, &storage_buf_cinfo);
    SDL_GPUBuffer *normals_buf  = SDL_CreateGPUBuffer(gpu_device, &storage_buf_cinfo);

    i32 indices_num = model_load(&vertices_buf, &normals_buf, &indices_buf, "monkey.glb");
    if (indices_num < 0) { return 0; }

    auto create_and_upload_storage_buffer = [](void* data, u32 size) -> SDL_GPUBuffer* {
        SDL_GPUBufferCreateInfo buf_cinfo = {
            .usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
            .size = size
        };
        SDL_GPUBuffer* buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);
        upload_data_to_gpu_buf(buf, data, size);
        return buf;
    };

    SDL_GPUBuffer *instance_accessor_buf = create_and_upload_storage_buffer(instance_accessors, sizeof(instance_accessors));
    SDL_GPUBuffer *vertex_accessor_buf   = create_and_upload_storage_buffer(vertex_accessors, sizeof(vertex_accessors));
    SDL_GPUBuffer *model_header_buf      = create_and_upload_storage_buffer(model_headers, sizeof(model_headers));
    SDL_GPUBuffer *transform_buf         = create_and_upload_storage_buffer(transform_list, sizeof(transform_list));
    SDL_GPUBuffer *view_buf              = create_and_upload_storage_buffer(&view_data, sizeof(view_data));
    SDL_GPUBuffer *color_buf             = create_and_upload_storage_buffer(material_color, sizeof(material_color));
    SDL_GPUBuffer *env_buf               = create_and_upload_storage_buffer(&env_data, sizeof(env_data));

    // depth texture
    SDL_GPUTextureCreateInfo depth_texture_cinfo = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = (u32)window_w,
        .height = (u32)window_h,
        .layer_count_or_depth = 1,
        .num_levels = 1
    };
    SDL_GPUTexture *depth_texture = SDL_CreateGPUTexture(gpu_device, &depth_texture_cinfo);

    bool app_running = true;
    while (app_running) {
        input_pre_poll();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { app_running = false; }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) { app_running = false; }
        }

        SDL_GPUCommandBuffer* command_buf = SDL_AcquireGPUCommandBuffer(gpu_device); {
            SDL_GPUTexture* swapchain_texture;
            u32 swapchain_width, swapchain_height;
            SDL_WaitAndAcquireGPUSwapchainTexture(command_buf, window,
                                                  &swapchain_texture, &swapchain_width, &swapchain_height);
            if (swapchain_texture == nullptr) {
                SDL_SubmitGPUCommandBuffer(command_buf);
                continue;
            }

            upload_data_to_gpu_buf(transform_buf, transform_list, sizeof(transform_list)); // FIXME this func not supposed to call every frame

            SDL_GPURenderPass *render_pass; {
                SDL_GPUColorTargetInfo color_target_info = {
                    .texture = swapchain_texture,
                    .clear_color = (SDL_FColor){0.8, 1, 0.8, 1},
                    .load_op = SDL_GPU_LOADOP_CLEAR,
                    .store_op = SDL_GPU_STOREOP_DONT_CARE
                };
                SDL_GPUDepthStencilTargetInfo depth_target_info = {
                    .texture = depth_texture,
                    .clear_depth = 1,
                    .load_op = SDL_GPU_LOADOP_CLEAR,
                    .store_op = SDL_GPU_STOREOP_STORE,
                };
                render_pass = SDL_BeginGPURenderPass(command_buf, &color_target_info, 1, &depth_target_info);

                SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

                SDL_GPUBuffer* vert_storage_buffers[] = {
                    instance_accessor_buf, // binding 0
                    vertex_accessor_buf,   // binding 1
                    model_header_buf,      // binding 2
                    vertices_buf,          // binding 3
                    normals_buf,           // binding 4
                    indices_buf,           // binding 5
                    transform_buf,         // binding 6
                    view_buf               // binding 7
                };
                SDL_BindGPUVertexStorageBuffers(render_pass, 0, vert_storage_buffers, 8);

                SDL_GPUBuffer* frag_storage_buffers[] = {
                    color_buf,             // binding 0
                    view_buf,              // binding 1
                    env_buf                // binding 2
                };
                SDL_BindGPUFragmentStorageBuffers(render_pass, 0, frag_storage_buffers, 3);

                i32 instance_num = sizeof(transform_list) / sizeof(Transform);
                SDL_DrawGPUPrimitives(render_pass, indices_num, instance_num, 0, 0);
            } SDL_EndGPURenderPass(render_pass);
        } SDL_SubmitGPUCommandBuffer(command_buf);
    }

    SDL_ReleaseGPUTexture(gpu_device, depth_texture);

    SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline);

    SDL_ReleaseGPUBuffer(gpu_device, vertices_buf);
    SDL_ReleaseGPUBuffer(gpu_device, normals_buf);
    SDL_ReleaseGPUBuffer(gpu_device, indices_buf);
    SDL_ReleaseGPUBuffer(gpu_device, transform_buf);
    SDL_ReleaseGPUBuffer(gpu_device, instance_accessor_buf);
    SDL_ReleaseGPUBuffer(gpu_device, vertex_accessor_buf);
    SDL_ReleaseGPUBuffer(gpu_device, model_header_buf);
    SDL_ReleaseGPUBuffer(gpu_device, view_buf);
    SDL_ReleaseGPUBuffer(gpu_device, color_buf);
    SDL_ReleaseGPUBuffer(gpu_device, env_buf);

    SDL_DestroyGPUDevice(gpu_device);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
