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

GLuint shader_load(const char *path, SHADER_TYPE shader_type) {
    string code = file_read(path);
    char *code_ptr = code.data(); // wtf is opengl smoking
    i32 code_len = code.size();

    GLenum type = (shader_type == SHADER_TYPE::VERTEX) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &code_ptr, &code_len);
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
    SHADER_VERTEX_WORLD_OBJECT = shader_load(file_source_path("shader/world_object.vert").c_str(), SHADER_TYPE::VERTEX);
    SHADER_FRAGMENT_WORLD_OBJECT = shader_load(file_source_path("shader/world_object.frag").c_str(), SHADER_TYPE::FRAGMENT);
}

}
