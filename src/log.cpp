module;

#include <cstdarg>
#include <cstdio>

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

inline void log(int level, const char* format, va_list args) {
    FILE* stream = (level >= LOG_WARN_LEVEL) ? stderr : stdout;

    fprintf(stream, "[%s] ", log_level_str(level));
    vfprintf(stream, format, args);
    fprintf(stream, "\n");
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
