module;

#include <cstdio>
#include <cstring>

export module file;

import define;
import mem;
import log;

export namespace asapage {

#if defined(_WIN32) || defined(__CYGWIN__)
constexpr char FILE_SEPARATOR = '\\';
#else
constexpr char FILE_SEPARATOR = '/';
#endif

char dir_project_raw[512];
char dir_source_raw[512];
char dir_asset_raw[512];
char dir_cache_raw[512];

void file_sys_init() {
    sprintf(dir_project_raw, "%s", _PROJECT_DIR);
    sprintf(dir_source_raw, "%s%c%s", _PROJECT_DIR, FILE_SEPARATOR, "src");
    sprintf(dir_asset_raw, "%s%c%s", _PROJECT_DIR, FILE_SEPARATOR, "asset");
    sprintf(dir_cache_raw, "%s%c%s", _PROJECT_DIR, FILE_SEPARATOR, "cache");
}

size_t file_to_native_path(char *path) {
    size_t path_len = strlen(path);
    if (path_len == 0) { return 0; }
    for (size_t i = 0; i < path_len; i++) {
        path[i] = ((path[i] == '/') | (path[i] == '\\')) ? FILE_SEPARATOR : path[i];
    }
    while (path_len > 1 && path[path_len - 1] == FILE_SEPARATOR) {
        path[path_len - 1] = '\0';
        path_len--;
    }
    return path_len;
}

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wvla-cxx-extension"
    #pragma clang diagnostic ignored "-Wvla"
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wvla"
#endif

void file_concat_path(char *dest, const char *src, size_t dest_size_cap = 0) { // dest_size_cap > 0 => turn on bounds check
    // size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);

    size_t native_dest_len = file_to_native_path(dest);

    char native_src[src_len + 1];
    mem_copy(native_src, src, src_len + 1);
    size_t native_src_len = file_to_native_path(native_src);

    size_t out_len = native_dest_len + native_src_len + 1; // +1 separator
    if (dest_size_cap > 0 && dest_size_cap < out_len + 1) { // +1 terminate
        log_err("file_concat_path: concat out of bounds: [%s] + [%s] is greater than [%d]", dest, src, dest_size_cap);
        return;
    }

    dest[native_dest_len] = FILE_SEPARATOR;
    mem_copy(dest + native_dest_len + 1, native_src, native_src_len);
    dest[out_len] = '\0';
}

void file_prepend_path(char *dest, const char *src, size_t dest_size_cap = 0) { // dest_size_cap > 0 => turn on bounds check
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);

    char native_dest[dest_len + 1];
    mem_copy(native_dest, dest, dest_len + 1);
    size_t native_dest_len = file_to_native_path(native_dest);

    char native_src[src_len + 1];
    mem_copy(native_src, src, src_len + 1);
    size_t native_src_len = file_to_native_path(native_src);

    size_t out_len = native_src_len + native_dest_len + 1; // +1 separator
    if (dest_size_cap > 0 && dest_size_cap < out_len + 1) { // +1 terminate
        log_err("file_prepend_path: concat out of bounds: [%s] + [%s] is greater than [%d]", src, dest, dest_size_cap);
        return;
    }

    mem_copy(dest, native_src, native_src_len);
    dest[native_src_len] = FILE_SEPARATOR;
    mem_copy(dest + native_src_len + 1, native_dest, native_dest_len);
    dest[out_len] = '\0';
}

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

void file_to_project_path(char *path, size_t path_size_cap = 0) {
    file_prepend_path(path, dir_project_raw, path_size_cap);
}
void file_to_source_path(char *path, size_t path_size_cap = 0) {
    file_prepend_path(path, dir_source_raw, path_size_cap);
}
void file_to_asset_path(char *path, size_t path_size_cap = 0) {
    file_prepend_path(path, dir_asset_raw, path_size_cap);
}
void file_to_cache_path(char *path, size_t path_size_cap = 0) {
    file_prepend_path(path, dir_cache_raw, path_size_cap);
}

}
