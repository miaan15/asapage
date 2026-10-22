module;

export module graphics;

export namespace asapage {
    //
    // // textures
    // SDL_GPUTexture *brick_texture_array; {
    //     constexpr i32 TEXTURE_WIDTH = 1024;
    //     constexpr i32 TEXTURE_HEIGHT = 1024;
    //     constexpr i32 TEXTURE_SIZE = TEXTURE_WIDTH * TEXTURE_HEIGHT;
    //     constexpr i32 TEXTURE_NUM = 2;
    //
    //     SDL_GPUTextureCreateInfo texture_cinfo = {
    //         .type = SDL_GPU_TEXTURETYPE_2D_ARRAY,
    //         .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
    //         .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    //         .width = TEXTURE_WIDTH,
    //         .height = TEXTURE_HEIGHT,
    //         .layer_count_or_depth = TEXTURE_NUM,
    //         .num_levels = 1
    //     };
    //     brick_texture_array = SDL_CreateGPUTexture(gpu_device, &texture_cinfo);
    //
    //     // paths
    //     char texture_path_list[TEXTURE_NUM][256] = { "brick_0.jpg", "brick_1.jpg" };
    //     for (i32 i = 0; i < TEXTURE_NUM; i++) {
    //         file_to_asset_path(texture_path_list[i]);
    //     }
    //
    //     // transfer buf, transfer
    //     SDL_GPUTransferBuffer *transfer_buf; {
    //         SDL_GPUTransferBufferCreateInfo transfer_buf_cinfo = {
    //             .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
    //             .size = 4 * TEXTURE_SIZE * TEXTURE_NUM
    //         };
    //         transfer_buf = SDL_CreateGPUTransferBuffer(gpu_device, &transfer_buf_cinfo);
    //     }
    //
    //     void *transfer_buf_mapped_ptr = SDL_MapGPUTransferBuffer(gpu_device, transfer_buf, false); {
    //         for (i32 i = 0; i < TEXTURE_NUM; i++) {
    //             SDL_Surface *surface = IMG_Load(texture_path_list[i]);
    //             if (!surface) {
    //                 SDL_Log("failed to load image %s: %s", texture_path_list[i], SDL_GetError());
    //                 continue;
    //             }
    //
    //             SDL_Surface *formatted_surface = surface;
    //             if (surface->format != SDL_PIXELFORMAT_RGBA32) {
    //                 formatted_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
    //                 SDL_DestroySurface(surface);
    //             }
    //
    //             mem_copy((char*)transfer_buf_mapped_ptr + (i * 4 * TEXTURE_SIZE), formatted_surface->pixels, 4 * TEXTURE_SIZE);
    //
    //             SDL_DestroySurface(formatted_surface);
    //         }
    //     } SDL_UnmapGPUTransferBuffer(gpu_device, transfer_buf);
    //
    //     SDL_GPUCommandBuffer *command_buf = SDL_AcquireGPUCommandBuffer(gpu_device); {
    //         SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buf); {
    //             for (i32 i = 0; i < TEXTURE_NUM; i++) {
    //                 SDL_GPUTextureTransferInfo src_info = {
    //                     .transfer_buffer = transfer_buf,
    //                     .offset = (u32)i * 4 * TEXTURE_SIZE,
    //                     .pixels_per_row = TEXTURE_WIDTH,
    //                     .rows_per_layer = TEXTURE_HEIGHT
    //                 };
    //                 SDL_GPUTextureRegion dest_region = {
    //                     .texture = brick_texture_array,
    //                     .layer = (u32)i,
    //                     .w = TEXTURE_WIDTH,
    //                     .h = TEXTURE_HEIGHT,
    //                     .d = 1
    //                 };
    //                 SDL_UploadToGPUTexture(copy_pass, &src_info, &dest_region, false);
    //             }
    //         } SDL_EndGPUCopyPass(copy_pass);
    //     } SDL_SubmitGPUCommandBuffer(command_buf);
    //
    //     SDL_ReleaseGPUTransferBuffer(gpu_device, transfer_buf);
    // }
    //
//
//
    // // sampler
    // SDL_GPUSampler *sampler; {
    //     SDL_GPUSamplerCreateInfo sampler_cinfo = {
    //         .min_filter = SDL_GPU_FILTER_LINEAR,
    //         .mag_filter = SDL_GPU_FILTER_LINEAR,
    //         .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR,
    //         .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    //         .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    //         .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    //         .max_lod = 1000.0f
    //     };
    //     sampler = SDL_CreateGPUSampler(gpu_device, &sampler_cinfo);
    // }
}
