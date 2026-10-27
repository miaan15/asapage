module;

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

export module define;

// MACRO
export using i8  = std::int8_t;
export using i16 = std::int16_t;
export using i32 = std::int32_t;
export using i64 = std::int64_t;

export using u8  = std::uint8_t;
export using u16 = std::uint16_t;
export using u32 = std::uint32_t;
export using u64 = std::uint64_t;

export using f32 = float;
export using f64 = double;

export using usize = std::size_t;
export using isize = std::ptrdiff_t;
export using uptr  = std::uintptr_t;
export using iptr  = std::intptr_t;

export using string = std::string;
export using string_view = std::string_view;

export namespace asapage {

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
#elif defined(__APPLE__) && defined(__MACH__)
constexpr PLATFORM PLATFORM = PLATFORM::MACOS;
#elif defined(__linux__)
constexpr PLATFORM PLATFORM = PLATFORM::LINUX;
#else
constexpr PLATFORM PLATFORM = PLATFORM::UNKNOWN;
#endif

#if defined(_DEBUG) || !defined(NDEBUG)
constexpr BUILD_TYPE BUILD_TYPE = BUILD_TYPE::DEBUG;
constexpr bool IS_DEBUG = true;
#else
constexpr BUILD_TYPE BUILD_TYPE = BUILD_TYPE::RELEASE;
constexpr bool IS_DEBUG = false;
#endif

}
