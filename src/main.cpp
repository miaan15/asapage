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

struct alignas(16) ViewUniformData {
    f32 screen_size[2];
    f32 __padding_0[2];

    f32 camera_pos[3];
    f32 __padding_1;

    f32 camera_target[3];
    f32 __padding_2;

    f32 camera_up[3];
    f32 camera_fov;
};

int main() {
    internal_init();

    file_sys_init();

    input_sys_init();

    // FIXME generalize this
    int window_w, window_h;
    SDL_GetWindowSizeInPixels(window, &window_w, &window_h);

    // uniform
    ViewUniformData view_uniform_data = {
        .screen_size = { (f32)window_w / (f32)window_h, 1 },
        .camera_pos = { 0, 5, 7 },
        .camera_target = { 0, 0, 0 },
        .camera_up = { 0, 1, 0 },
        .camera_fov = 20.0 * 3.14159 / 180.0
    };

    // shader + pipeline
    char vertex_shader_path[512] = "mvp_shader.vert";
    file_to_asset_path(vertex_shader_path, 512);
    char fragment_shader_path[512] = "mvp_shader.frag";
    file_to_asset_path(fragment_shader_path, 512);

    SDL_GPUShader *vertex_shader = create_shader_glsl_file(vertex_shader_path, SHADER_STAGE::VERTEX, 0, 0, 0, 1);
    SDL_GPUShader *fragment_shader = create_shader_glsl_file(fragment_shader_path, SHADER_STAGE::FRAGMENT, 1, 0, 0, 0);
    if (vertex_shader == nullptr || fragment_shader == nullptr) { assert(false); }

    SDL_GPUGraphicsPipeline *pipeline;
    init_pipeline(&pipeline, vertex_shader, fragment_shader);

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    // buffers
    SDL_GPUBuffer *vertices_buf; {
        SDL_GPUBufferCreateInfo buf_cinfo = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = 1 << 10 << 10 // 10 MB
        };
        vertices_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);
    }
    SDL_GPUBuffer *indices_buf; {
        SDL_GPUBufferCreateInfo buf_cinfo = {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = 1 << 10 << 10 // 10 MB
        };
        indices_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);
    }
    SDL_GPUBuffer *normals_buf; {
        SDL_GPUBufferCreateInfo buf_cinfo = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = 1 << 10 << 10 // 10 MB
        };
        normals_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);
    }

    i32 indices_num = model_load(&vertices_buf, &normals_buf, &indices_buf, "box.glb");
    if (indices_num < 0) { return 0; }

    // textures
    SDL_GPUTexture *brick_texture_array; {
        constexpr i32 TEXTURE_WIDTH = 1024;
        constexpr i32 TEXTURE_HEIGHT = 1024;
        constexpr i32 TEXTURE_SIZE = TEXTURE_WIDTH * TEXTURE_HEIGHT;
        constexpr i32 TEXTURE_NUM = 2;

        SDL_GPUTextureCreateInfo texture_cinfo = {
            .type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
            .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
            .width = TEXTURE_WIDTH,
            .height = TEXTURE_HEIGHT,
            .layer_count_or_depth = TEXTURE_NUM,
            .num_levels = 1
        };
        brick_texture_array = SDL_CreateGPUTexture(gpu_device, &texture_cinfo);

        // paths
        char texture_path_list[TEXTURE_NUM][256] = { "brick_0.jpg", "brick_1.jpg" };
        for (i32 i = 0; i < TEXTURE_NUM; i++) {
            file_to_asset_path(texture_path_list[i]);
        }

        // transfer buf, transfer
        SDL_GPUTransferBuffer *transfer_buf; {
            SDL_GPUTransferBufferCreateInfo transfer_buf_cinfo = {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = 4 * TEXTURE_SIZE * TEXTURE_NUM
            };
            transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);
        }

        void *transfer_buf_mapped_ptr = SDL_MapGPUTransferBuffer(gpu_device, transfer_buf, false); {
            for (i32 i = 0; i < TEXTURE_NUM; i++) {
                SDL_Surface *surface = IMG_Load(texture_path_list[i]);
                if (!surface) {
                    SDL_Log("failed to load image %s: %s", texture_path_list[i], SDL_GetError());
                    continue;
                }

                SDL_Surface *formatted_surface = surface;
                if (surface->format != SDL_PIXELFORMAT_RGBA32) {
                    formatted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
                    SDL_DestroySurface(surface);
                }

                mem_copy((char*)transfer_buf_mapped_ptr + (i * 4 * TEXTURE_SIZE), formatted_surface->pixels, 4 * TEXTURE_SIZE);

                SDL_DestroySurface(formatted_surface);
            }
        } SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buf);

        SDL_GPUCommandBuffer *command_buf = SDL_AcquireGPUCommandBuffer(gpu_device); {
            SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buf); {
                for (i32 i = 0; i < TEXTURE_NUM; i++) {
                    SDL_GPUTextureTransferInfo src_info = {
                        .transfer_buffer = transfer_buf,
                        .offset = (u32)i * 4 * TEXTURE_SIZE,
                        .pixels_per_row = TEXTURE_WIDTH,
                        .rows_per_layer = TEXTURE_HEIGHT
                    };
                    SDL_GPUTextureRegion dest_region = {
                        .texture = brick_texture_array,
                        .layer = (u32)i,
                        .w = TEXTURE_WIDTH,
                        .h = TEXTURE_HEIGHT,
                        .d = 1
                    };
                    SDL_UploadToGPUTexture(copy_pass, &src_info, &dest_region, false);
                }
            } SDL_EndGPUCopyPass(copy_pass);
        } SDL_SubmitGPUCommandBuffer(command_buf);

        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buf);
    }

    // sampler
    SDL_GPUSampler *sampler; {
        SDL_GPUSamplerCreateInfo sampler_cinfo = {
            .min_filter = SDL_GPU_FILTER_LINEAR,
            .mag_filter = SDL_GPU_FILTER_LINEAR,
            .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
            .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
            .max_lod = 1000.0f
        };
        sampler = SDL_CreateGPUSampler(gpu_device, &sampler_cinfo);
    }

    // FIXME
    Transform transform_list[1] = {}; {
        transform_list[0] = (Transform) {
            .translation = { 0, 0, 0 },
            .rotation = { 0, 0, 0, 1 },
            .scale = { 1, 1, 1 },
        };
    }
    SDL_GPUBuffer *transform_buf; {
        SDL_GPUBufferCreateInfo buf_cinfo = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(transform_list)
        };
        transform_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);
    }

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

        // FIXME test
        constexpr f32 SPEED = 0.1;
        if (input_is_key_on(SCANCODE::W)) { view_uniform_data.camera_pos[2] += SPEED; }
        if (input_is_key_on(SCANCODE::S)) { view_uniform_data.camera_pos[2] -= SPEED; }
        if (input_is_key_on(SCANCODE::A)) { view_uniform_data.camera_pos[0] -= SPEED; }
        if (input_is_key_on(SCANCODE::D)) { view_uniform_data.camera_pos[0] += SPEED; }
        if (input_is_key_on(SCANCODE::LEFT))  { view_uniform_data.camera_target[0] -= SPEED; }
        if (input_is_key_on(SCANCODE::RIGHT)) { view_uniform_data.camera_target[0] += SPEED; }
        if (input_is_key_on(SCANCODE::UP))    { view_uniform_data.camera_target[1] += SPEED; }
        if (input_is_key_on(SCANCODE::DOWN))  { view_uniform_data.camera_target[1] -= SPEED; }

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

                // uniform
                SDL_PushGPUVertexUniformData(command_buf, 0, &view_uniform_data, sizeof(view_uniform_data));

                // bind vertex
                SDL_GPUBufferBinding vertex_buf_binding_list_buf[3] = {}; {
                    vertex_buf_binding_list_buf[0] = {
                        .buffer = vertices_buf,
                        .offset = 0
                    };
                    vertex_buf_binding_list_buf[1] = {
                        .buffer = normals_buf,
                        .offset = 0
                    };
                    vertex_buf_binding_list_buf[2] = {
                        .buffer = transform_buf,
                        .offset = 0
                    };
                }
                SDL_BindGPUVertexBuffers(render_pass, 0, vertex_buf_binding_list_buf, 3);

                // bind fragment
                SDL_GPUTextureSamplerBinding texture_sampler_binding[1] = {}; {
                    texture_sampler_binding[0] = {
                        .texture = brick_texture_array,
                        .sampler = sampler
                    };
                }
                SDL_BindGPUFragmentSamplers(render_pass, 0, texture_sampler_binding, 1);

                // bind index
                SDL_GPUBufferBinding index_buf_binding = {
                    .buffer = indices_buf,
                    .offset = 0
                };
                SDL_BindGPUIndexBuffer(render_pass, &index_buf_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

                // draw
                u32 transform_num = sizeof(transform_list) / sizeof(Transform);
                SDL_DrawGPUIndexedPrimitives(render_pass, indices_num, transform_num, 0, 0, 0);

            } SDL_EndGPURenderPass(render_pass);
        } SDL_SubmitGPUCommandBuffer(command_buf);
    }

    SDL_ReleaseGPUTexture(gpu_device, depth_texture);
    SDL_ReleaseGPUTexture(gpu_device, brick_texture_array);
    SDL_ReleaseGPUSampler(gpu_device, sampler);

    SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline);
    SDL_ReleaseGPUBuffer(gpu_device, vertices_buf);
    SDL_ReleaseGPUBuffer(gpu_device, normals_buf);
    SDL_ReleaseGPUBuffer(gpu_device, indices_buf);

    SDL_ReleaseGPUBuffer(gpu_device, transform_buf);

    SDL_DestroyGPUDevice(gpu_device);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
