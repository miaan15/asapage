#include <cassert>
#include <SDL3/SDL.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>

import define;
import mem;
import internal;
import context;
import file;
import input;
import shader;

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
        .camera_pos = { 0, 10, -10 },
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
    SDL_GPUShader *fragment_shader = create_shader_glsl_file(fragment_shader_path, SHADER_STAGE::FRAGMENT, 0, 0, 0, 0);
    if (vertex_shader == nullptr || fragment_shader == nullptr) { assert(false); }

    SDL_GPUGraphicsPipeline *pipeline;
    init_pipeline(&pipeline, vertex_shader, fragment_shader);

    SDL_ReleaseGPUShader(gpu_device, vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, fragment_shader);

    // buffers
    SDL_GPUBuffer *vertices_buf; {
        SDL_GPUBufferCreateInfo buf_cinfo = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = 10 << 10 << 10 // 10 MB
        };
        vertices_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);
    }
    SDL_GPUBuffer *indices_buf; {
        SDL_GPUBufferCreateInfo buf_cinfo = {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = 10 << 10 << 10 // 10 MB
        };
        indices_buf = SDL_CreateGPUBuffer(gpu_device, &buf_cinfo);
    }

    i32 indices_num = model_load(&vertices_buf, &indices_buf, "monkey.glb");
    if (indices_num < 0) { return 0; }

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
                SDL_GPUColorTargetInfo color_target_info = (SDL_GPUColorTargetInfo) {
                    .texture = swapchain_texture,
                    .clear_color = (SDL_FColor){0.8, 1, 0.8, 1},
                    .load_op = SDL_GPU_LOADOP_CLEAR,
                    .store_op = SDL_GPU_STOREOP_DONT_CARE
                };
                SDL_GPUDepthStencilTargetInfo depth_target_info = (SDL_GPUDepthStencilTargetInfo) {
                    .texture = depth_texture,
                    .clear_depth = 1,
                    .load_op = SDL_GPU_LOADOP_CLEAR,
                    .store_op = SDL_GPU_STOREOP_STORE,
                };
                render_pass = SDL_BeginGPURenderPass(command_buf, &color_target_info, 1, &depth_target_info);

                SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

                // uniform
                SDL_PushGPUVertexUniformData(command_buf, 0, &view_uniform_data, sizeof(view_uniform_data));

                // draw
                SDL_GPUBufferBinding vertex_buf_binding_list_buf[2] = {}; {
                    vertex_buf_binding_list_buf[0] = (SDL_GPUBufferBinding) {
                        .buffer = vertices_buf,
                        .offset = 0
                    };
                    vertex_buf_binding_list_buf[1] = (SDL_GPUBufferBinding) {
                        .buffer = transform_buf,
                        .offset = 0
                    };
                }
                SDL_BindGPUVertexBuffers(render_pass, 0, vertex_buf_binding_list_buf, 2);

                SDL_GPUBufferBinding index_buf_binding = {
                    .buffer = indices_buf,
                    .offset = 0
                };
                SDL_BindGPUIndexBuffer(render_pass, &index_buf_binding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

                u32 transform_num = sizeof(transform_list) / sizeof(Transform);
                SDL_DrawGPUIndexedPrimitives(render_pass, indices_num, transform_num, 0, 0, 0);

            } SDL_EndGPURenderPass(render_pass);
        } SDL_SubmitGPUCommandBuffer(command_buf);
    }

    SDL_ReleaseGPUTexture(gpu_device, depth_texture);

    SDL_ReleaseGPUGraphicsPipeline(gpu_device, pipeline);
    SDL_ReleaseGPUBuffer(gpu_device, vertices_buf);
    SDL_ReleaseGPUBuffer(gpu_device, indices_buf);

    SDL_ReleaseGPUBuffer(gpu_device, transform_buf);

    SDL_DestroyGPUDevice(gpu_device);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
