#pragma once

enum struct PLATFORM {
    UNKNOWN,
    WINDOWS,
    LINUX,
    MACOS
};

enum struct BUILD_TYPE {
    DEBUG,
    RELEASE
};

#if defined(_WIN32) || defined(_WIN64)
    constexpr PLATFORM PLATFORM = PLATFORM::WINDOWS;
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__) && defined(__MACH__)
    constexpr PLATFORM PLATFORM = PLATFORM::MACOS;
    #define PLATFORM_MACOS
#elif defined(__linux__)
    constexpr PLATFORM PLATFORM = PLATFORM::LINUX;
    #define PLATFORM_LINUX
#else
    constexpr PLATFORM PLATFORM = PLATFORM::UNKNOWN;
    #define PLATFORM_UNKNOWN
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
    constexpr BUILD_TYPE BUILD_TYPE = BUILD_TYPE::DEBUG;
    constexpr bool IS_DEBUG = true;
    #define BUILD_DEBUG
#else
    constexpr BUILD_TYPE BUILD_TYPE = BUILD_TYPE::RELEASE;
    constexpr bool IS_DEBUG = false;
    #define BUILD_RELEASE
#endif

