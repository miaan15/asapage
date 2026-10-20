#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <SDL3/SDL.h>

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

import vertex_box;

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

struct Transform {
    f32 translation[3];
    f32 rotation[4];
    f32 scale[3];
};

int main() {
    internal_init();

    file_sys_init();

    input_sys_init();

    // FIXME generalize this
    int window_w, window_h;
    SDL_GetWindowSizeInPixels(window, &window_w, &window_h);

    // Render GPU stuff
    constexpr u32 VERTEX_BUFFER_SIZE = 1 << 10 << 10; // ~1 MB
    SDL_GPUBuffer *vertex_buf;
    SDL_GPUBuffer *transform_buf;
    SDL_GPUGraphicsPipeline *mvp_pipeline;

    // Data
    ViewUniformData view_uniform_data = {
        .screen_size = { (f32)window_w / (f32)window_h, 1 },
        .camera_pos = { 0, 10, -10 },
        .camera_target = { 0, 0, 0 },
        .camera_up = { 0, 1, 0 },
        .camera_fov = 20.0 * 3.14159 / 180.0
    };

    // FIXME
    Transform transform_list[5] = {};
    transform_list[0] = (Transform) {
        .translation = { 0, 0, 0 },
        .rotation = { 0, 0, 0, 1 },
        .scale = { 1, 1, 1 },
    };
    transform_list[1] = (Transform) {
        .translation = { 1, 0, 0 },
        .rotation = { 0, 0, 0, 1 },
        .scale = { 1, 2, 1 },
    };
    transform_list[2] = (Transform) {
        .translation = { 3, 0, 0 },
        .rotation = { 0, 0, 0, 1 },
        .scale = { 2, 1, 1 },
    };
    transform_list[3] = (Transform) {
        .translation = { 1, 1, 0 },
        .rotation = { 0, 0, 0, 1 },
        .scale = { 1, 2, 1 },
    };
    transform_list[4] = (Transform) {
        .translation = { 2, 2, 0 },
        .rotation = { 0, 0, 0, 1 },
        .scale = { 1, 2, 2 },
    };

    // Pipelines
    {
        // Shaders
        char vertex_shader_path[512] = "mvp_shader.vert";
        file_to_asset_path(vertex_shader_path, 512);
        char fragment_shader_path[512] = "mvp_shader.frag";
        file_to_asset_path(fragment_shader_path, 512);

        SDL_GPUShader *vertex_shader = create_shader_glsl_file(vertex_shader_path, SHADER_STAGE::VERTEX, 0, 0, 0, 1);
        SDL_GPUShader *fragment_shader = create_shader_glsl_file(fragment_shader_path, SHADER_STAGE::FRAGMENT, 0, 0, 0, 0);
        if (vertex_shader == nullptr || fragment_shader == nullptr) { assert(false); }

        // Create pipeline
        SDL_GPUGraphicsPipelineCreateInfo create_info = {};
        create_info.vertex_shader = vertex_shader;
        create_info.fragment_shader = fragment_shader;
        create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // Vertex input
        // - VertexBufferDescription
        SDL_GPUVertexBufferDescription vertex_buffer_decs_list_buf[2] = {};
        vertex_buffer_decs_list_buf[0] = (SDL_GPUVertexBufferDescription) {
            .slot = 0,
            .pitch = 3 * sizeof(f32),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        };
        vertex_buffer_decs_list_buf[1] = (SDL_GPUVertexBufferDescription) {
            .slot = 1,
            .pitch = sizeof(Transform),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
        };

        // - VertexAttribute
        SDL_GPUVertexAttribute vertex_attribute_list_buf[4] = {};
        vertex_attribute_list_buf[0] = (SDL_GPUVertexAttribute) { // layout(location = 0) in vec3 i_pos;
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0,
        };
        vertex_attribute_list_buf[1] = (SDL_GPUVertexAttribute) { // layout(location = 1) in vec3 i_translation;
            .location = 1,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Transform, translation),
        };
        vertex_attribute_list_buf[2] = (SDL_GPUVertexAttribute) { // layout(location = 2) in vec4 i_rotation;
            .location = 2,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(Transform, rotation),
        };
        vertex_attribute_list_buf[3] = (SDL_GPUVertexAttribute) { // layout(location = 3) in vec3 i_scale;
            .location = 3,
            .buffer_slot = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Transform, scale),
        };

        // >
        SDL_GPUVertexInputState vertex_input_state = (SDL_GPUVertexInputState) {
            .vertex_buffer_descriptions = vertex_buffer_decs_list_buf, .num_vertex_buffers = 2,
            .vertex_attributes = vertex_attribute_list_buf, .num_vertex_attributes = 4,
        };
        create_info.vertex_input_state = vertex_input_state;

        // PipelineTargetInfo
        // - ColorTargetDescription
        SDL_GPUColorTargetDescription color_target_desc_list_buf[1] = {};
        color_target_desc_list_buf[0] = (SDL_GPUColorTargetDescription) {
            .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
        };

        // >
        SDL_GPUGraphicsPipelineTargetInfo pipeline_target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
            .color_target_descriptions = color_target_desc_list_buf, .num_color_targets = 1,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM, .has_depth_stencil_target = true
        };
        create_info.target_info = pipeline_target_info;

        // Rasterizer State
        SDL_GPURasterizerState rasterizer_state = (SDL_GPURasterizerState) {
            .cull_mode = SDL_GPU_CULLMODE_BACK,
            .front_face = SDL_GPU_FRONTFACE_CLOCKWISE // because of I using left-hand coor
        };
        create_info.rasterizer_state = rasterizer_state;

        // Depth Stencil State
        SDL_GPUDepthStencilState depth_stencil_state = (SDL_GPUDepthStencilState) {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = true,
            .enable_depth_write = true
        };
        create_info.depth_stencil_state = depth_stencil_state;

        // >
        mvp_pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &create_info);

        // Clean up shaders
        SDL_ReleaseGPUShader(gpu_device, vertex_shader);
        SDL_ReleaseGPUShader(gpu_device, fragment_shader);
    }

    // Buffers
    {
        SDL_GPUBufferCreateInfo vertex_buf_create_info = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(box_verticies)
        };
        vertex_buf = SDL_CreateGPUBuffer(gpu_device, &vertex_buf_create_info);

        SDL_GPUBufferCreateInfo transform_buf_create_info = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(transform_list)
        };
        transform_buf = SDL_CreateGPUBuffer(gpu_device, &transform_buf_create_info);
    }

    // Copy vertex data to GPU
    {
        // Transfer buffer
        SDL_GPUTransferBufferCreateInfo transfer_buf_create_info = (SDL_GPUTransferBufferCreateInfo) {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(box_verticies),
        };
        SDL_GPUTransferBuffer *transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_create_info);

        // Map
        void *transfer_buf_mapped_ptr = SDL_MapGPUTransferBuffer(gpu_device, transfer_buf, true);
        mem_copy(transfer_buf_mapped_ptr, box_verticies, sizeof(box_verticies));
        SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buf);

        //
        SDL_GPUCommandBuffer* upload_command_buf = SDL_AcquireGPUCommandBuffer(gpu_device);

        // Copy pass
        SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_command_buf);

        SDL_GPUTransferBufferLocation transfer_buf_location = (SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buf,
            .offset = 0
        };
        SDL_GPUBufferRegion buf_region = (SDL_GPUBufferRegion) {
            .buffer = vertex_buf,
            .offset = 0,
            .size = sizeof(box_verticies)
        };
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buf_location, &buf_region, false);

        SDL_EndGPUCopyPass(copy_pass);

        SDL_SubmitGPUCommandBuffer(upload_command_buf);

        SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buf);
    }

    // Depth texture
    SDL_GPUTextureCreateInfo depth_texture_create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = (u32)window_w,
        .height = (u32)window_h,
        .layer_count_or_depth = 1,
        .num_levels = 1
    };
    SDL_GPUTexture *depth_texture = SDL_CreateGPUTexture(gpu_device, &depth_texture_create_info);

        // FIXME
        SDL_GPUTransferBuffer *transform_transfer_buf; {
            SDL_GPUTransferBufferCreateInfo transfer_buf_create_info = (SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = sizeof(transform_list),
            };
            transform_transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_create_info);
        }

    bool app_running = true;
    while (app_running) {
        input_pre_poll();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { app_running = false; }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) { app_running = false; }
        }

        // Command buffer
        SDL_GPUCommandBuffer* command_buf = SDL_AcquireGPUCommandBuffer(gpu_device);

        // Swapchain texture
        SDL_GPUTexture* swapchain_texture;
        u32 swapchain_width, swapchain_height;
        SDL_WaitAndAcquireGPUSwapchainTexture(
            command_buf, window,
            &swapchain_texture, &swapchain_width, &swapchain_height
        );
        if (swapchain_texture == NULL) {
            SDL_SubmitGPUCommandBuffer(command_buf);
            continue;
        }

        // Pass Transform to GPU // FIXME
        {
            // Map
            void *transfer_buf_mapped_ptr = SDL_MapGPUTransferBuffer(gpu_device, transform_transfer_buf, true);
            mem_copy(transfer_buf_mapped_ptr, &transform_list, sizeof(transform_list));
            SDL_UnmapGPUTransferBuffer(gpu_device, transform_transfer_buf);

            // Copy pass
            SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buf);

            SDL_GPUTransferBufferLocation transfer_buf_location = (SDL_GPUTransferBufferLocation) {
                .transfer_buffer = transform_transfer_buf,
                .offset = 0
            };
            SDL_GPUBufferRegion buf_region = (SDL_GPUBufferRegion) {
                .buffer = transform_buf,
                .offset = 0,
                .size = sizeof(transform_list)
            };
            SDL_UploadToGPUBuffer(copy_pass, &transfer_buf_location, &buf_region, true);

            SDL_EndGPUCopyPass(copy_pass);
        }

        // Render pass
        SDL_GPURenderPass *render_pass;

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
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
        };

        render_pass = SDL_BeginGPURenderPass(command_buf, &color_target_info, 1, &depth_target_info);

        // Bind pipeline
        SDL_BindGPUGraphicsPipeline(render_pass, mvp_pipeline);

        // Uniform
        SDL_PushGPUVertexUniformData(command_buf, 0, &view_uniform_data, sizeof(view_uniform_data));

        // Bind draw
        SDL_GPUBufferBinding buf_binding_list_buf[2] = {};
        buf_binding_list_buf[0] = (SDL_GPUBufferBinding) {
            .buffer = vertex_buf,
            .offset = 0
        };
        buf_binding_list_buf[1] = (SDL_GPUBufferBinding) {
            .buffer = transform_buf,
            .offset = 0
        };
        SDL_BindGPUVertexBuffers(render_pass, 0, buf_binding_list_buf, 2);

        u32 num_verticies = sizeof(box_verticies) / (3 * sizeof(f32));
        u32 num_transform = 5;
        SDL_DrawGPUPrimitives(render_pass, num_verticies, num_transform, 0, 0);

        //
        SDL_EndGPURenderPass(render_pass);

        // > Submit
        SDL_SubmitGPUCommandBuffer(command_buf);
    }

    // Clean up
    SDL_ReleaseGPUTexture(gpu_device, depth_texture);

    SDL_ReleaseGPUGraphicsPipeline(gpu_device, mvp_pipeline);
    SDL_ReleaseGPUBuffer(gpu_device, vertex_buf);
    SDL_ReleaseGPUBuffer(gpu_device, transform_buf);

    SDL_ReleaseGPUTransferBuffer(gpu_device, transform_transfer_buf); // FIXME

    SDL_DestroyGPUDevice(gpu_device);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
