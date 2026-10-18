#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <SDL3/SDL.h>

import define;
import internal;
import context;
import file_sys;
import shader;

using namespace asapage;

struct VertexPoint {
    f32 x, y, z, r, g, b;
};

int main() {
    internal_init();

    file_sys_init();

    // Render GPU stuff
    constexpr u32 VERTEX_BUFFER_SIZE = 100 << 10 << 10; // ~100 MB
    SDL_GPUBuffer *mvp_vertex_buf;

    SDL_GPUGraphicsPipeline *mvp_pipeline;

    // Shaders
    char mvp_vertex_shader_path[512] = "mvp_shader.vert";
    file_to_asset_path(mvp_vertex_shader_path, 512);
    char mvp_fragment_shader_path[512] = "mvp_shader.frag";
    file_to_asset_path(mvp_fragment_shader_path, 512);

    SDL_GPUShader *mvp_vertex_shader = create_shader_glsl_file(mvp_vertex_shader_path, SHADER_STAGE_VERTEX, 0, 0, 0, 1);
    SDL_GPUShader *mvp_fragment_shader = create_shader_glsl_file(mvp_fragment_shader_path, SHADER_STAGE_FRAGMENT, 0, 0, 0, 0);

    // Pipelines
    {
        SDL_GPUGraphicsPipelineCreateInfo create_info = {};
        create_info.vertex_shader = mvp_vertex_shader;
        create_info.fragment_shader = mvp_fragment_shader;
        create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

        // Vertex input
        // - VertexBufferDescription
        SDL_GPUVertexBufferDescription vertex_buffer_decs_list_buf[1] = {};
        vertex_buffer_decs_list_buf[0] = (SDL_GPUVertexBufferDescription) {
            .slot = 0,
            .pitch = sizeof(VertexPoint),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0
        };

        // - VertexAttribute
        SDL_GPUVertexAttribute vertex_attribute_list_buf[2] = {};
        vertex_attribute_list_buf[0] = (SDL_GPUVertexAttribute) {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0,
        };
        vertex_attribute_list_buf[1] = (SDL_GPUVertexAttribute) {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 3 * sizeof(float),
        };

        // > Submit
        SDL_GPUVertexInputState vertex_input_state = (SDL_GPUVertexInputState) {
            .vertex_buffer_descriptions = vertex_buffer_decs_list_buf,
            .num_vertex_buffers = 1,
            .vertex_attributes = vertex_attribute_list_buf,
            .num_vertex_attributes = 2,
        };
        create_info.vertex_input_state = vertex_input_state;

        // PipelineTargetInfo
        // - ColorTargetDescription
        SDL_GPUColorTargetDescription color_target_desc_list_buf[1] = {};
        color_target_desc_list_buf[0] = (SDL_GPUColorTargetDescription) {
            .format = SDL_GetGPUSwapchainTextureFormat(gpu_device, window),
        };

        // > Submit
        SDL_GPUGraphicsPipelineTargetInfo pipeline_target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
            .color_target_descriptions = color_target_desc_list_buf,
            .num_color_targets = 1
        };
        create_info.target_info = pipeline_target_info;

        mvp_pipeline = SDL_CreateGPUGraphicsPipeline(gpu_device, &create_info);
    }

    // Clean up shaders
    SDL_ReleaseGPUShader(gpu_device, mvp_vertex_shader);
    SDL_ReleaseGPUShader(gpu_device, mvp_fragment_shader);

    // Buffers
    {
        SDL_GPUBufferCreateInfo vertex_buffer_create_info = (SDL_GPUBufferCreateInfo) {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = VERTEX_BUFFER_SIZE
        };
        mvp_vertex_buf = SDL_CreateGPUBuffer(gpu_device, &vertex_buffer_create_info);
    }

    bool app_running = true;
    while (app_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) { app_running = false; }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) { app_running = false; }

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

            // > Submit
            SDL_SubmitGPUCommandBuffer(command_buf);
        }
    }

    // Clean up GPU
    SDL_ReleaseGPUGraphicsPipeline(gpu_device, mvp_pipeline);

    SDL_ReleaseGPUBuffer(gpu_device, mvp_vertex_buf);

    SDL_DestroyGPUDevice(gpu_device);

    // Clean up SDL
    SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
