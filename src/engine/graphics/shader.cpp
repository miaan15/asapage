module;

#include <cstdlib>
#include <glad.h>

export module graphics_shader;

import define;
import file;
import log;

export namespace asapage {

enum struct SHADER_TYPE { VERTEX, FRAGMENT };

GLuint SHADER_VERTEX_WORLD_OBJECT = -1;
GLuint SHADER_FRAGMENT_WORLD_OBJECT = -1;

GLuint shader_world_object_load(const char *path, const char *header_code, int header_code_len, SHADER_TYPE shader_type) {
    string code = file_read(path);
    const char *code_ptrs[2] = { header_code, code.c_str() }; // wtf is opengl smoking
    i32 code_lens[2] = { header_code_len, (int)code.length() };

    GLenum type = (shader_type == SHADER_TYPE::VERTEX) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 2, code_ptrs, code_lens);
    glCompileShader(shader);

    bool success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, (GLint*)&success);
    if (!success) {
        i32 log_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);

        if (log_len > 0) {
            char *log = (char*)malloc(log_len);
            glGetShaderInfoLog(shader, log_len, nullptr, log);
            log_err("shader_load: compilation failed: path = [%s]:\n%s", path, log);
            free(log);
        } else {
            log_err("shader_load: compilation failed: unknown error: path = [%s]", log, path);
        }

        glDeleteShader(shader);

        return 0;
    }

    log_debug("Load shader: path = [%s], type = [%d]", path, shader_type);

    return shader;
}

void shader_sys_init() {
    string world_object_header_code = file_read(file_source_path("shader/world_object_header.glsl").c_str());

    SHADER_VERTEX_WORLD_OBJECT = shader_world_object_load(file_source_path("shader/world_object.vert").c_str(),
                                                          world_object_header_code.c_str(), world_object_header_code.length(),
                                                          SHADER_TYPE::VERTEX);
    SHADER_FRAGMENT_WORLD_OBJECT = shader_world_object_load(file_source_path("shader/world_object.frag").c_str(),
                                                            world_object_header_code.c_str(), world_object_header_code.length(),
                                                            SHADER_TYPE::FRAGMENT);
}

}
