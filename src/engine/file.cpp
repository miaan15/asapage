module;

#include <cstdio>
#include <cstring>
#include <raylib.h>

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

inline char project_dir[512];
inline char source_dir[512];
inline char asset_dir[512];
inline char cache_dir[512];

string file_native_path(string_view path) {
    string res; res.reserve(path.length());

    i32 path_separator_count = 0;
    for (size_t i = 0; i < path.length(); i++) {
        if (path[i] == '/' || path[i] == '\\') {
            path_separator_count++;
        } else {
            if (path_separator_count > 0) {
                res += FILE_SEPARATOR;
            }
            res += path[i];
            path_separator_count = 0;
        }
    }

    if (path_separator_count > 0) {
        res += FILE_SEPARATOR;
    }

    return res;
}

string file_concat_path(string_view dest, string_view src) {
    string res;
    res.reserve(dest.size() + 1 + src.size());
    res += dest;
    res += FILE_SEPARATOR;
    res += src;
    return file_native_path(res);
}

void file_sys_init() {
    // somehow llvm complains about global inline string var, so use char[] instead

    strcpy(project_dir, _PROJECT_DIR);
    sprintf(source_dir, "%s%c%s", project_dir, FILE_SEPARATOR, "src");
    sprintf(asset_dir, "%s%c%s", project_dir, FILE_SEPARATOR, "asset");
    sprintf(cache_dir, "%s%c%s", project_dir, FILE_SEPARATOR, "cache");
}

string file_project_path(string_view path) { return file_concat_path(project_dir, path); }
string file_source_path(string_view path)  { return file_concat_path(source_dir, path); }
string file_asset_path(string_view path)   { return file_concat_path(asset_dir, path); }
string file_cache_path(string_view path)   { return file_concat_path(cache_dir, path); }

string file_read(string_view path) {
    string native_path = file_native_path(path);

    FILE *fp = fopen(native_path.c_str(), "r");

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    string res; res.resize(size);

    fread(res.data(), 1, size, fp);

    fclose(fp);

    return res;
}

}
