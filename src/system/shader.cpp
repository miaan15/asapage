module;

#include <cstdio>
#include <cstdlib>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>

export module shader;

import define;
import mem;
import internal;
import file;

export namespace asapage {

enum struct SHADER_STAGE {
    VERTEX = SDL_GPU_SHADERSTAGE_VERTEX,
    FRAGMENT = SDL_GPU_SHADERSTAGE_FRAGMENT,
};

enum struct SHADER_COMPILE_OPTIMIZATION {
    NONE, SIZE, SPEED,
};

SDL_GPUShader *shader_load(const char *path, bool use_cache, SHADER_STAGE stage,
                           u32 sampler_num, u32 storage_texture_num, u32 storage_buf_num, u32 uniform_buf_num,
                           SHADER_COMPILE_OPTIMIZATION optimization = SHADER_COMPILE_OPTIMIZATION::SPEED) {
    char asset_path[512];
    strcpy(asset_path, path);
    file_to_asset_path(asset_path, 512);

    char cache_dir_path[512] = "";
    file_to_cache_path(cache_dir_path, sizeof(cache_dir_path));

    char cache_shader_dir[512];
    strcpy(cache_shader_dir, cache_dir_path);
    file_concat_path(cache_shader_dir, "shader", sizeof(cache_shader_dir));

    SDL_CreateDirectory(cache_shader_dir);

    char cache_shader_path[512];
    strcpy(cache_shader_path, cache_shader_dir);
    file_concat_path(cache_shader_path, path, sizeof(cache_shader_path));
    strcat(cache_shader_path, ".spv");

    size_t spirv_code_len = 0;
    void *spirv_code_buf = nullptr;

    if (use_cache) {
        spirv_code_buf = SDL_LoadFile(cache_shader_path, &spirv_code_len);
    }

    if (spirv_code_buf == nullptr) {
        const char *stage_flag_string = (stage == SHADER_STAGE::VERTEX) ? "vertex" : "fragment";
        const char *optimization_flag_string = "0";
        if (optimization == SHADER_COMPILE_OPTIMIZATION::SIZE) { optimization_flag_string = "s"; }
        if (optimization == SHADER_COMPILE_OPTIMIZATION::SPEED) { optimization_flag_string = ""; }

        char command[512];
        sprintf(command, "glslc -fshader-stage=%s -O%s -x glsl \"%s\" -o \"%s\"",
                stage_flag_string, optimization_flag_string, asset_path, cache_shader_path);

        int compile_res = system(command);
        if (compile_res != 0) {
            printf("glslc compilation failed: %s\n", command);
            return nullptr;
        }

        spirv_code_buf = SDL_LoadFile(cache_shader_path, &spirv_code_len);
        if (spirv_code_buf == nullptr) {
            printf("failed to load cached shader: %s\n", cache_shader_path);
            return nullptr;
        }
    }

    SDL_GPUShaderCreateInfo create_info = {
        .code_size = spirv_code_len,
        .code = (const u8*)spirv_code_buf,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = (SDL_GPUShaderStage)stage,
        .num_samplers = sampler_num,
        .num_storage_textures = storage_texture_num,
        .num_storage_buffers = storage_buf_num,
        .num_uniform_buffers = uniform_buf_num
    };
    SDL_GPUShader *shader = SDL_CreateGPUShader(gpu_device, &create_info);

    SDL_free(spirv_code_buf);

    return shader;
}

}
