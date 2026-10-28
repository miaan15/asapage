module;

#include <cstdio>
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

inline string project_dir;
inline string source_dir;
inline string asset_dir;
inline string cache_dir;

string file_native_path(string_view path) {
    string result { path };
    if (result.empty()) {
        return result;
    }

    for (char &c : result) {
        if (c == '/' || c == '\\') {
            c = FILE_SEPARATOR;
        }
    }

    while (result.length() > 1 && result.back() == FILE_SEPARATOR) {
        result.pop_back();
    }

    return result;
}

string file_concat_path(string_view dest, string_view src) {
    string native_dest = file_native_path(dest);
    string native_src = file_native_path(src);

    if (native_dest.empty()) return native_src;
    if (native_src.empty()) return native_dest;

    string result;
    result.reserve(native_dest.size() + 1 + native_src.size());
    result += native_dest;
    result += FILE_SEPARATOR;
    result += native_src;
    return result;
}

string file_prepend_path(string_view dest, string_view src) {
    return file_concat_path(src, dest);
}

void file_sys_init() {
    project_dir = _PROJECT_DIR;
    source_dir  = file_concat_path(project_dir, "src");
    asset_dir   = file_concat_path(project_dir, "asset");
    cache_dir   = file_concat_path(project_dir, "cache");
}

string file_project_path(string_view path) { return file_concat_path(project_dir, path); }
string file_source_path(string_view path)  { return file_concat_path(source_dir, path); }
string file_asset_path(string_view path)   { return file_concat_path(asset_dir, path); }
string file_cache_path(string_view path)   { return file_concat_path(cache_dir, path); }

string_view file_extension(string_view path) {
    i32 dot_pos = path.find_last_of('.');
    i32 separator_pos = path.find_last_of("/\\");

    if (dot_pos == string_view::npos || (separator_pos != string_view::npos && dot_pos < separator_pos)) {
        return {};
    }
    return path.substr(dot_pos);
}

string_view file_name(string_view path) {
    i32 separator_pos = path.find_last_of("/\\");
    if (separator_pos == string_view::npos) return path;
    return path.substr(separator_pos + 1);
}

string_view file_dir(string_view path) {
    i32 separator_pos = path.find_last_of("/\\");
    if (separator_pos == string_view::npos) return {};
    return path.substr(0, separator_pos);
}

string_view file_stem(string_view path) {
    string_view filename = file_name(path);
    i32 dot_pos = filename.find_last_of('.');
    if (dot_pos == string_view::npos) return filename;
    return filename.substr(0, dot_pos);
}

bool file_exists_file(string_view path) {
    string native = file_native_path(path);
    return FileExists(native.c_str());
}

bool file_exists_dir(string_view path) {
    string native = file_native_path(path);
    return DirectoryExists(native.c_str());
}

i64 file_size(string_view path) {
    string native = file_native_path(path);
    if (!FileExists(native.c_str())) return 0;
    return GetFileLength(native.c_str());
}

bool file_make_dir(string_view path) {
    string native = file_native_path(path);
    int res = MakeDirectory(native.c_str());
    if (res != 0) {
        log_err("file_make_dir [%s] failed", native.c_str());
        return false;
    }
    return true;
}

bool file_make_file(string_view path) {
    string native = file_native_path(path);

    string_view parent = file_dir(native);
    if (!parent.empty() && !file_exists_dir(parent)) {
        file_make_dir(parent);
    }

    bool ok = SaveFileData(native.c_str(), (void*)"", 0);
    if (!ok) {
        log_err("file_make_file [%s] failed", native.c_str());
    }
    return ok;
}

bool file_remove(string_view path) {
    string native = file_native_path(path);

    bool ok = (remove(native.c_str()) == 0);
    if (!ok) {
        log_err("file_remove [%s] failed", native.c_str());
    }
    return ok;
}

}
