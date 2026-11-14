module;

#include "define.hpp"
#include <cstdarg>
#include <cstdio>

#if defined(PLATFORM_LINUX) && defined(BUILD_DEBUG)
    #define LOG_USE_BACKTRACE
#endif

#ifdef LOG_USE_BACKTRACE
#include <dlfcn.h>
#include <execinfo.h>
#include <unistd.h>
#include <cstdint>
#include <cstring>
#endif

export module log;

import define;

export namespace asapage {

// only use in debug
inline bool log_disable_trace = false;
inline bool log_disable_debug = false;
inline bool log_disable_info  = false;
inline bool log_disable_warn  = false;
inline bool log_disable_err   = false;

enum {
    LOG_TRACE_LEVEL,
    LOG_DEBUG_LEVEL,
    LOG_INFO_LEVEL,
    LOG_WARN_LEVEL,
    LOG_ERR_LEVEL,
};

inline const char* log_level_str(int level) {
    switch (level) {
        case LOG_TRACE_LEVEL: return "\033[36mTRACE\033[0m";
        case LOG_DEBUG_LEVEL: return "\033[34mDEBUG\033[0m";
        case LOG_INFO_LEVEL:  return "\033[32mINFO\033[0m";
        case LOG_WARN_LEVEL:  return "\033[33mWARN\033[0m";
        case LOG_ERR_LEVEL:   return "\033[31mERROR\033[0m";
        default:              return "UNKNOWN";
    }
}

inline void print_stacktrace(FILE* stream) {
#ifdef LOG_USE_BACKTRACE
    void* array[32];
    int size = backtrace(array, 32);

    int _size = size > 3 + 3 ? 3 + 3 : size;
    for (int i = 3; i < _size; ++i) {
        Dl_info info;
        if (dladdr(array[i], &info) && info.dli_fname) {
            uintptr_t addr = (uintptr_t)array[i] - (uintptr_t)info.dli_fbase;

            char cmd[512];
            snprintf(cmd, sizeof(cmd), "addr2line -C -f -e \"%s\" 0x%lx", info.dli_fname, (unsigned long)addr);

            FILE* fp = popen(cmd, "r");
            if (fp) {
                char func[256] = {0};
                char file[256] = {0};
                if (fgets(func, sizeof(func), fp) && fgets(file, sizeof(file), fp)) {
                    func[strcspn(func, "\r\n")] = 0;
                    file[strcspn(file, "\r\n")] = 0;
                    fprintf(stream, " %d. %-32.32s - %s\n", i - 2, func, file);
                } else {
                    fprintf(stream, " %d. %-32.32s - (+0x%lx)\n", i - 2, info.dli_fname, (unsigned long)addr);
                }
                pclose(fp);
            } else {
                fprintf(stream, " %d. 0x%lx\n", i - 2, (unsigned long)array[i]);
            }
        }
    }
#endif
}

inline void log(int level, const char* format, va_list args) {
    FILE* stream = (level >= LOG_WARN_LEVEL) ? stderr : stdout;

    fprintf(stream, "[%s] ", log_level_str(level));
    vfprintf(stream, format, args);
    fprintf(stream, "\n");

#ifdef LOG_USE_BACKTRACE
    if (level >= LOG_WARN_LEVEL) { print_stacktrace(stream); }
#endif
}

inline void log_trace([[maybe_unused]] const char* format, ...) {
    if constexpr (IS_DEBUG) {
        if (log_disable_trace) { return; }

        va_list args;
        va_start(args, format);
        log(LOG_TRACE_LEVEL, format, args);
        va_end(args);
    }
}

inline void log_debug([[maybe_unused]] const char* format, ...) {
    if constexpr (IS_DEBUG) {
        if (log_disable_debug) { return; }

        va_list args;
        va_start(args, format);
        log(LOG_DEBUG_LEVEL, format, args);
        va_end(args);
    }
}

inline void log_info([[maybe_unused]] const char* format, ...) {
    if constexpr (IS_DEBUG) {
        if (log_disable_info) { return; }
    }

    va_list args;
    va_start(args, format);
    log(LOG_INFO_LEVEL, format, args);
    va_end(args);
}

inline void log_warn([[maybe_unused]] const char* format, ...) {
    if constexpr (IS_DEBUG) {
        if (log_disable_warn) { return; }
    }

    va_list args;
    va_start(args, format);
    log(LOG_WARN_LEVEL, format, args);
    va_end(args);
}

inline void log_err([[maybe_unused]] const char* format, ...) {
    if constexpr (IS_DEBUG) {
        if (log_disable_err) { return; }
    }

    va_list args;
    va_start(args, format);
    log(LOG_ERR_LEVEL, format, args);
    va_end(args);
}

}
