module;

#include <cstdio>
#include <cstdlib>
#include <SDL3/SDL_gpu.h>
#include <cstring>

#if PLATFORM == PLATFORM_WINDOWS
    #include <windows.h>
    #include <d3dcompiler.h>
    #pragma comment(lib, "d3dcompiler.lib")
#endif

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

SDL_GPUShader *shader_load(const char *path, SHADER_STAGE stage,
                           i32 sampler_num, i32 storage_texture_num, i32 storage_buf_num, i32 uniform_buf_num,
                           SHADER_COMPILE_OPTIMIZATION optimization = SHADER_COMPILE_OPTIMIZATION::SPEED) {
#if PLATFORM == PLATFORM_WINDOWS

    char true_path[512];
    strcpy(true_path, path);
    file_to_asset_path(true_path, 512);

    FILE *f_src = fopen(true_path, "rb");
    if (!f_src) {
        printf("Failed to open HLSL file: %s", true_path);
        return NULL;
    }

    fseek(f_src, 0, SEEK_END);
    size_t hlsl_source_len = ftell(f_src);
    fseek(f_src, 0, SEEK_SET);

    char *hlsl_source_buf = (char *)malloc(hlsl_source_len + 1);
    if (!hlsl_source_buf) {
        fclose(f_src);
        printf("Out of memory allocating source buffer for: %s", true_path);
        return NULL;
    }

    fread(hlsl_source_buf, 1, hlsl_source_len, f_src);
    fclose(f_src);
    hlsl_source_buf[hlsl_source_len] = '\0';

    const char *target_profile = (stage == SHADER_STAGE::VERTEX) ? "vs_5_1" : "ps_5_1";

    UINT compile_flags = D3DCOMPILE_ENABLE_STRICTNESS;
    if (optimization == SHADER_COMPILE_OPTIMIZATION::SIZE) {
        compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
    } else {
        compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
    }

    ID3DBlob *code_blob = NULL;
    ID3DBlob *error_blob = NULL;

    HRESULT _res = D3DCompile(
        hlsl_source_buf, hlsl_source_len,
        true_path,
        NULL,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main",
        target_profile,
        compile_flags, 0,
        &code_blob, &error_blob
    );

    free(hlsl_source_buf);

    if (FAILED(_res)) {
        if (error_blob) {
            printf("HLSL Compile Error (%s):\n%s", true_path, (char *)error_blob->GetBufferPointer());
            error_blob->Release();
        } else {
            printf("Failed to compile HLSL shader: %s", true_path);
        }
        return NULL;
    }

    if (error_blob) {
        error_blob->Release();
    }

    SDL_GPUShaderCreateInfo create_info = {
        .code_size = code_blob->GetBufferSize(),
        .code = (const Uint8 *)code_blob->GetBufferPointer(),
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_DXBC,
        .stage = (SDL_GPUShaderStage)stage,
        .num_samplers = (u32)sampler_num,
        .num_storage_textures = (u32)storage_texture_num,
        .num_storage_buffers = (u32)storage_buf_num,
        .num_uniform_buffers = (u32)uniform_buf_num
    };

    SDL_GPUShader *shader = SDL_CreateGPUShader(gpu_device, &create_info);

    code_blob->Release();

    return shader;

#else

    printf("HLSL direct compilation is not supported on non-Windows platforms: %s", path);
    return NULL;

#endif
}

}
