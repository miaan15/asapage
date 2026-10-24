module;

#include <cstdio>
#include <cstdlib>
#include <SDL3/SDL_gpu.h>

export module shader;

import define;
import mem;
import internal;

export namespace asapage {

enum struct SHADER_STAGE {
    VERTEX = SDL_GPU_SHADERSTAGE_VERTEX,
    FRAGMENT = SDL_GPU_SHADERSTAGE_FRAGMENT,
};

enum struct SHADER_COMPILE_OPTIMIZATION {
    NONE, SIZE, SPEED,
};

SDL_GPUShader *create_shader_glsl_file(const char *glsl_code_path,
                                       SHADER_STAGE stage,
                                       u32 sampler_num, u32 storage_texture_num, u32 storage_buf_num, u32 uniform_buf_num,
                                       SHADER_COMPILE_OPTIMIZATION optimization = SHADER_COMPILE_OPTIMIZATION::SPEED) {
    const char *stage_flag_string = stage == SHADER_STAGE::VERTEX ? "vertex" : "fragment";
    const char *optimization_flag_string = "0";
    if (optimization == SHADER_COMPILE_OPTIMIZATION::SIZE) { optimization_flag_string = "s"; }
    if (optimization == SHADER_COMPILE_OPTIMIZATION::SPEED) { optimization_flag_string = ""; }

    char command[1024];
    sprintf(command, "glslc -fshader-stage=%s -O%s -x glsl %s -o -", stage_flag_string, optimization_flag_string, glsl_code_path);

    FILE *fp_glslc_command = popen(command, "rb");
    if (fp_glslc_command == NULL) {
        printf("command wrong: %s", command);
        return NULL;
    }

    usize spirv_code_cap = 1024;
    usize spirv_code_len = 0;
    u8 *spirv_code_buf = (u8*)malloc(spirv_code_cap);

    constexpr size_t FREAD_DATA_READ_SIZE = 1024;
    u8 fread_data_read_buf[FREAD_DATA_READ_SIZE];
    i32 fread_data_read_len;
    while (true) {
        fread_data_read_len = fread(fread_data_read_buf, 1, FREAD_DATA_READ_SIZE, fp_glslc_command);

        if (spirv_code_len + fread_data_read_len > spirv_code_cap) {
            spirv_code_cap *= 2;
            spirv_code_buf = (u8*)realloc(spirv_code_buf, spirv_code_cap);
        }
        mem_copy(spirv_code_buf + spirv_code_len, fread_data_read_buf, fread_data_read_len);
        spirv_code_len += fread_data_read_len;

        if (fread_data_read_len < FREAD_DATA_READ_SIZE) { break; }
    }

    SDL_GPUShaderCreateInfo create_info = {
        .code_size = spirv_code_len,
        .code = spirv_code_buf,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = (SDL_GPUShaderStage)stage,
        .num_samplers = sampler_num,
        .num_storage_textures = storage_texture_num,
        .num_storage_buffers = storage_buf_num,
        .num_uniform_buffers = uniform_buf_num
    };
    SDL_GPUShader *shader = SDL_CreateGPUShader(gpu_device, &create_info);

    free(spirv_code_buf);

    return shader;
}

}
