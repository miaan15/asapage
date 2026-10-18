#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <SDL3/SDL.h>

import define;
import internal;
import context;
import file_sys;
import shader;

import vertex_box;

using namespace asapage;

struct alignas(16) TransformUniform {
    f32 u_translation[3];
    f32 _pad0;

    f32 u_rotation[4];

    f32 u_scale[3];
    f32 _pad1;
};

struct alignas(16) ColorUniform {
    f32 u_color[3];
    f32 _pad0;
};

int main() {
    internal_init();

    file_sys_init();

    // Render GPU stuff
    constexpr u32 VERTEX_BUFFER_SIZE = 1 << 10 << 10; // ~1 MB
    SDL_GPUBuffer *mvp_vertex_buf;
    SDL_GPUTransferBuffer *mvp_vertex_transfer_buf;
    SDL_GPUGraphicsPipeline *mvp_pipeline;

    // Pipelines
    {
        // Shaders
        char vertex_shader_path[512] = "mvp_shader.vert";
        file_to_asset_path(vertex_shader_path, 512);
        char fragment_shader_path[512] = "mvp_shader.frag";
        file_to_asset_path(fragment_shader_path, 512);

        SDL_GPUShader *vertex_shader = create_shader_glsl_file(vertex_shader_path, SHADER_STAGE_VERTEX, 0, 0, 0, 2);
        SDL_GPUShader *fragment_shader = create_shader_glsl_file(fragment_shader_path, SHADER_STAGE_FRAGMENT, 0, 0, 0, 0);
        if (vertex_shader == nullptr || fragment_shader == nullptr) { assert(false); }

        // Create pipeline
        SDL_GPUGraphicsPipelineCreateInfo create_info = {};
        create_info.vertex_shader = vertex_shader;
        create_info.fragment_shader = fragment_shader;
        create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // Vertex input
        // - VertexBufferDescription
        SDL_GPUVertexBufferDescription vertex_buffer_decs_list_buf[1] = {};
        vertex_buffer_decs_list_buf[0] = (SDL_GPUVertexBufferDescription) {
            .slot = 0,
            .pitch = 3 * sizeof(f32),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        };

        // - VertexAttribute
        SDL_GPUVertexAttribute vertex_attribute_list_buf[1] = {};
        vertex_attribute_list_buf[0] = (SDL_GPUVertexAttribute) { // layout(location = 0) in vec3 i_pos;
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0,
        };

        // >
        SDL_GPUVertexInputState vertex_input_state = (SDL_GPUVertexInputState) {
            .vertex_buffer_descriptions = vertex_buffer_decs_list_buf, .num_vertex_buffers = 1,
            .vertex_attributes = vertex_attribute_list_buf, .num_vertex_attributes = 1,
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
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
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
        // Buffer
        SDL_GPUBufferCreateInfo buf_create_info = (SDL_GPUBufferCreateInfo) {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = VERTEX_BUFFER_SIZE
        };
        mvp_vertex_buf = SDL_CreateGPUBuffer(gpu_device, &buf_create_info);

        // Transfer buffers
        SDL_GPUTransferBufferCreateInfo transfer_buf_create_info = (SDL_GPUTransferBufferCreateInfo) {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = VERTEX_BUFFER_SIZE,
        };
        mvp_vertex_transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_create_info);
    }
    // Copy vertex data to GPU
    {
        // Map
        void *transfer_buf_mapped_ptr = SDL_MapGPUTransferBuffer(gpu_device, mvp_vertex_transfer_buf, true);

        memcpy(transfer_buf_mapped_ptr, box_verticies, sizeof(box_verticies));

        SDL_UnmapGPUTransferBuffer(gpu_device, mvp_vertex_transfer_buf);

        //
        SDL_GPUCommandBuffer* upload_command_buf = SDL_AcquireGPUCommandBuffer(gpu_device);

        // Copy pass
        SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_command_buf);

        SDL_GPUTransferBufferLocation transfer_buf_location = (SDL_GPUTransferBufferLocation) {
            .transfer_buffer = mvp_vertex_transfer_buf,
            .offset = 0
        };
        SDL_GPUBufferRegion buf_region = (SDL_GPUBufferRegion) {
            .buffer = mvp_vertex_buf,
            .offset = 0,
            .size = sizeof(box_verticies)
        };
        SDL_UploadToGPUBuffer(copy_pass, &transfer_buf_location, &buf_region, true);

        SDL_EndGPUCopyPass(copy_pass);

        SDL_SubmitGPUCommandBuffer(upload_command_buf);
    }

    // FIXME generalize this
    int window_w, window_h;
    SDL_GetWindowSizeInPixels(window, &window_w, &window_h);

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

    TransformUniform transform_data = { // FIXME
        .u_translation = {0, 0, 0},
        .u_rotation = {0, 0, 0, 1},
        .u_scale = {1, 1, 1}
    };
    ColorUniform color_data = { // FIXME
        .u_color = {1, 1, 1}
    };

    f32 x_rot = 0, y_rot = 0; // FIXME test rotate

    bool app_running = true;
    while (app_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { app_running = false; }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) { app_running = false; }

            // FIXME test rotate
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.scancode == SDL_SCANCODE_W) { x_rot -= 3.14159 / 180.0; }
                if (event.key.scancode == SDL_SCANCODE_A) { y_rot -= 3.14159 / 180.0; }
                if (event.key.scancode == SDL_SCANCODE_S) { x_rot += 3.14159 / 180.0; }
                if (event.key.scancode == SDL_SCANCODE_D) { y_rot += 3.14159 / 180.0; }
            }
        }

        { // FIXME test rotate
            f32 cx = cos(x_rot * 0.5f), sx = sin(x_rot * 0.5f);
            f32 cy = cos(y_rot * 0.5f), sy = sin(y_rot * 0.5f);
            transform_data.u_rotation[0] = sx * cy;
            transform_data.u_rotation[1] = cx * sy;
            transform_data.u_rotation[2] = -sx * sy;
            transform_data.u_rotation[3] = cx * cy;
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

        // Render pass
        SDL_GPURenderPass *render_pass;

        SDL_GPUColorTargetInfo color_target_info = {};
        color_target_info.texture = swapchain_texture;
        color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
        color_target_info.clear_color = (SDL_FColor){0.9, 0.9, 0.9, 1};

        SDL_GPUDepthStencilTargetInfo depth_target_info = {};
        depth_target_info.texture = depth_texture;
        depth_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        depth_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_target_info.clear_depth = 1;

        render_pass = SDL_BeginGPURenderPass(command_buf, &color_target_info, 1, &depth_target_info);

        SDL_BindGPUGraphicsPipeline(render_pass, mvp_pipeline);

        // Uniform
        SDL_PushGPUVertexUniformData(command_buf, 0, &transform_data, sizeof(transform_data));
        SDL_PushGPUVertexUniformData(command_buf, 1, &color_data, sizeof(color_data));

        // Draw
        SDL_GPUBufferBinding buf_binding_list_buf[1] = {};
        buf_binding_list_buf[0] = (SDL_GPUBufferBinding) {
            .buffer = mvp_vertex_buf,
            .offset = 0
        };
        SDL_BindGPUVertexBuffers(render_pass, 0, buf_binding_list_buf, 1);

        SDL_DrawGPUPrimitives(render_pass, sizeof(box_verticies) / sizeof(f32), 1, 0, 0);

        //
        SDL_EndGPURenderPass(render_pass);

        // > Submit
        SDL_SubmitGPUCommandBuffer(command_buf);
    }

    // Clean up
    SDL_ReleaseGPUTexture(gpu_device, depth_texture);
    SDL_ReleaseGPUGraphicsPipeline(gpu_device, mvp_pipeline);
    SDL_ReleaseGPUBuffer(gpu_device, mvp_vertex_buf);
    SDL_ReleaseGPUTransferBuffer(gpu_device, mvp_vertex_transfer_buf);
    SDL_DestroyGPUDevice(gpu_device);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
