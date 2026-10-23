module;

#include <cmath>

export module math;

import define;

export namespace asapage {

// ============================================================================================================================
inline constexpr f32 MATH_PI          = 3.14159265358979323846;
inline constexpr f32 MATH_TWO_PI      = 6.28318530717958647692;
inline constexpr f32 MATH_HALF_PI     = 1.57079632679489661923;
inline constexpr f32 MATH_QUARTER_PI  = 0.78539816339744830962;
inline constexpr f32 MATH_ONE_OVER_PI = 0.31830988618379067154;
inline constexpr f32 MATH_E           = 2.71828182845904523536;
inline constexpr f32 MATH_SQRT_2      = 1.414213562373095048;
inline constexpr f32 MATH_EPSILON     = 1e-5;
inline constexpr f32 MATH_DEG_TO_RAD  = MATH_PI / 180;
inline constexpr f32 MATH_RAD_TO_DEG  = 180 / MATH_PI;

inline f32 math_to_radians(f32 degrees) { return degrees * MATH_DEG_TO_RAD; }
inline f32 math_to_degrees(f32 radians) { return radians * MATH_RAD_TO_DEG; }

inline f32 math_min(f32 a, f32 b) { return a < b ? a : b; }
inline f32 math_max(f32 a, f32 b) { return a > b ? a : b; }
inline f32 math_abs(f32 v)        { return fabs(v); }
inline f32 math_sqrt(f32 v)       { return sqrtf(v); }
inline f32 math_cbrt(f32 v)       { return cbrtf(v); }
inline f32 math_sign(f32 v)       { return (v > 0) - (v < 0); }
inline f32 math_floor(f32 v)      { return floor(v); }
inline f32 math_ceiling(f32 v)    { return ceil(v); }

inline f32 math_cos(f32 v)        { return cosf(v); }
inline f32 math_sin(f32 v)        { return sinf(v); }
inline f32 math_tan(f32 v)        { return tanf(v); }
inline f32 math_acos(f32 v)       { return acosf(v); }
inline f32 math_asin(f32 v)       { return asinf(v); }
inline f32 math_atan(f32 v)       { return atanf(v); }

inline f32 math_clamp(f32 v, f32 min_val, f32 max_val) {
    return v < min_val ? min_val : (v > max_val ? max_val : v);
}

inline f32 math_clamp01(f32 v) {
    return math_clamp(v, 0, 1);
}

inline f32 math_lerp(f32 a, f32 b, f32 t) {
    return a + (b - a) * t;
}

inline f32 math_inverse_lerp(f32 a, f32 b, f32 v) {
    return (a != b) ? math_clamp01((v - a) / (b - a)) : 0;
}

inline f32 math_remap(f32 in_min, f32 in_max, f32 out_min, f32 out_max, f32 v) {
    f32 t = math_inverse_lerp(in_min, in_max, v);
    return math_lerp(out_min, out_max, t);
}

inline bool math_approximate(f32 a, f32 b, f32 eps = MATH_EPSILON) {
    return math_abs(a - b) <= eps;
}

inline f32 math_step(f32 edge, f32 x) {
    return x < edge ? 0 : 1;
}

inline f32 math_smoothstep(f32 edge0, f32 edge1, f32 x) {
    f32 t = math_clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3 - 2 * t);
}

inline f32 math_smootherstep(f32 edge0, f32 edge1, f32 x) {
    f32 t = math_clamp01((x - edge0) / (edge1 - edge0));
    return t * t * t * (t * (t * 6 - 15) + 10);
}

inline f32 math_repeat(f32 t, f32 length) {
    return math_clamp(t - math_floor(t / length) * length, 0, length);
}

inline f32 math_ping_pong(f32 t, f32 length) {
    t = math_repeat(t, length * 2);
    return length - math_abs(t - length);
}

// LINEAR
// ============================================================================================================================
struct Vec2 {
    union {
        f32 raw[2];
        struct { f32 x, y; };
    };
};

struct Vec3 {
    union {
        f32 raw[3];
        struct { f32 x, y, z; };
    };
};

struct Vec4 {
    union {
        f32 raw[4];
        struct { f32 x, y, z, w; };
    };
};
using Quat = Vec4;

struct Mat2 {
    union {
        f32 raw[2][2];
        struct { Vec2 vecs[2]; };
    };
};

struct Mat3 {
    union {
        f32 raw[3][3];
        struct { Vec3 vecs[3]; };
    };
};

struct Mat4 {
    union {
        f32 raw[4][4];
        struct { Vec4 vecs[4]; };
    };
};

// CONSTANTS
// vec2
inline constexpr Vec2 VEC2_ZERO     = { 0, 0 };
inline constexpr Vec2 VEC2_ONE      = { 1, 1 };
inline constexpr Vec2 VEC2_LEFT     = {-1, 0 };
inline constexpr Vec2 VEC2_RIGHT    = { 1, 0 };
inline constexpr Vec2 VEC2_UP       = { 0, 1 };
inline constexpr Vec2 VEC2_DOWN     = { 0,-1 };

// vec3
inline constexpr Vec3 VEC3_ZERO     = { 0, 0, 0 };
inline constexpr Vec3 VEC3_ONE      = { 1, 1, 1 };
inline constexpr Vec3 VEC3_RIGHT    = { 1, 0, 0 };
inline constexpr Vec3 VEC3_LEFT     = {-1, 0, 0 };
inline constexpr Vec3 VEC3_UP       = { 0, 1, 0 };
inline constexpr Vec3 VEC3_DOWN     = { 0,-1, 0 };
inline constexpr Vec3 VEC3_FORWARD  = { 0, 0,-1 };
inline constexpr Vec3 VEC3_BACK     = { 0, 0, 1 };

// vec4
inline constexpr Vec4 VEC4_ZERO     = { 0, 0, 0, 0 };
inline constexpr Vec4 VEC4_ONE      = { 1, 1, 1, 1 };
inline constexpr Quat QUAT_IDENTITY = { 0, 0, 0, 1 };

// mat2
inline constexpr Mat2 MAT2_ZERO     = { .raw = {{0, 0}, {0, 0}} };
inline constexpr Mat2 MAT2_IDENTITY = { .raw = {{1, 0}, {0, 1}} };

// mat3
inline constexpr Mat3 MAT3_ZERO     = { .raw = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}} };
inline constexpr Mat3 MAT3_IDENTITY = { .raw = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}} };

// mat4
inline constexpr Mat4 MAT4_ZERO     = { .raw = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}} };
inline constexpr Mat4 MAT4_IDENTITY = { .raw = {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}} };

// OPERATOR OVERLOAD
// vec2
inline Vec2 operator+(const Vec2& a, const Vec2& b) { return { a.x + b.x, a.y + b.y }; }
inline Vec2 operator-(const Vec2& a, const Vec2& b) { return { a.x - b.x, a.y - b.y }; }
inline Vec2 operator*(const Vec2& a, const Vec2& b) { return { a.x * b.x, a.y * b.y }; }
inline Vec2 operator/(const Vec2& a, const Vec2& b) { return { a.x / b.x, a.y / b.y }; }

inline Vec2 operator*(const Vec2& v, f32 s) { return { v.x * s, v.y * s }; }
inline Vec2 operator*(f32 s, const Vec2& v) { return { v.x * s, v.y * s }; }
inline Vec2 operator/(const Vec2& v, f32 s) { return { v.x / s, v.y / s }; }

inline Vec2& operator+=(Vec2& a, const Vec2& b) { a.x += b.x; a.y += b.y; return a; }
inline Vec2& operator-=(Vec2& a, const Vec2& b) { a.x -= b.x; a.y -= b.y; return a; }
inline Vec2& operator*=(Vec2& a, const Vec2& b) { a.x *= b.x; a.y *= b.y; return a; }
inline Vec2& operator/=(Vec2& a, const Vec2& b) { a.x /= b.x; a.y /= b.y; return a; }
inline Vec2& operator*=(Vec2& v, f32 s) { v.x *= s; v.y *= s; return v; }
inline Vec2& operator/=(Vec2& v, f32 s) { v.x /= s; v.y /= s; return v; }

inline Vec2 operator-(Vec2 v) { return {-v.x, -v.y}; }

inline bool operator==(const Vec2& a, const Vec2& b) { return a.x == b.x && a.y == b.y; }
inline bool operator!=(const Vec2& a, const Vec2& b) { return !(a == b); }

// vec3
inline Vec3 operator+(const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
inline Vec3 operator*(const Vec3& a, const Vec3& b) { return { a.x * b.x, a.y * b.y, a.z * b.z }; }
inline Vec3 operator/(const Vec3& a, const Vec3& b) { return { a.x / b.x, a.y / b.y, a.z / b.z }; }

inline Vec3 operator*(const Vec3& v, f32 s) { return { v.x * s, v.y * s, v.z * s }; }
inline Vec3 operator*(f32 s, const Vec3& v) { return { v.x * s, v.y * s, v.z * s }; }
inline Vec3 operator/(const Vec3& v, f32 s) { return { v.x / s, v.y / s, v.z / s }; }

inline Vec3& operator+=(Vec3& a, const Vec3& b) { a.x += b.x; a.y += b.y; a.z += b.z; return a; }
inline Vec3& operator-=(Vec3& a, const Vec3& b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; return a; }
inline Vec3& operator*=(Vec3& a, const Vec3& b) { a.x *= b.x; a.y *= b.y; a.z *= b.z; return a; }
inline Vec3& operator/=(Vec3& a, const Vec3& b) { a.x /= b.x; a.y /= b.y; a.z /= b.z; return a; }
inline Vec3& operator*=(Vec3& v, f32 s) { v.x *= s; v.y *= s; v.z *= s; return v; }
inline Vec3& operator/=(Vec3& v, f32 s) { v.x /= s; v.y /= s; v.z /= s; return v; }

inline Vec3 operator-(Vec3 v) { return {-v.x, -v.y, -v.z}; }

inline bool operator==(const Vec3& a, const Vec3& b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
inline bool operator!=(const Vec3& a, const Vec3& b) { return !(a == b); }

// vec4
inline Vec4 operator+(const Vec4& a, const Vec4& b) { return { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
inline Vec4 operator-(const Vec4& a, const Vec4& b) { return { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
inline Vec4 operator*(const Vec4& a, const Vec4& b) { return { a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w }; }
inline Vec4 operator/(const Vec4& a, const Vec4& b) { return { a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }; }

inline Vec4 operator*(const Vec4& v, f32 s) { return { v.x * s, v.y * s, v.z * s, v.w * s }; }
inline Vec4 operator*(f32 s, const Vec4& v) { return { v.x * s, v.y * s, v.z * s, v.w * s }; }
inline Vec4 operator/(const Vec4& v, f32 s) { return { v.x / s, v.y / s, v.z / s, v.w / s }; }

inline Vec4& operator+=(Vec4& a, const Vec4& b) { a.x += b.x; a.y += b.y; a.z += b.z; a.w += b.w; return a; }
inline Vec4& operator-=(Vec4& a, const Vec4& b) { a.x -= b.x; a.y -= b.y; a.z -= b.z; a.w -= b.w; return a; }
inline Vec4& operator*=(Vec4& a, const Vec4& b) { a.x *= b.x; a.y *= b.y; a.z *= b.z; a.w *= b.w; return a; }
inline Vec4& operator/=(Vec4& a, const Vec4& b) { a.x /= b.x; a.y /= b.y; a.z /= b.z; a.w /= b.w; return a; }
inline Vec4& operator*=(Vec4& v, f32 s) { v.x *= s; v.y *= s; v.z *= s; v.w *= s; return v; }
inline Vec4& operator/=(Vec4& v, f32 s) { v.x /= s; v.y /= s; v.z /= s; v.w /= s; return v; }

inline Vec4 operator-(Vec4 v) { return {-v.x, -v.y, -v.z, -v.w}; }

inline bool operator==(const Vec4& a, const Vec4& b) { return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w; }
inline bool operator!=(const Vec4& a, const Vec4& b) { return !(a == b); }

// mat2
inline Vec2 operator*(const Mat2& m, const Vec2& v) {
    return {
        m.raw[0][0] * v.x + m.raw[1][0] * v.y,
        m.raw[0][1] * v.x + m.raw[1][1] * v.y
    };
}

inline Mat2 operator*(const Mat2& a, const Mat2& b) {
    return Mat2{ .raw = {
        { a.raw[0][0] * b.raw[0][0] + a.raw[1][0] * b.raw[0][1], a.raw[0][1] * b.raw[0][0] + a.raw[1][1] * b.raw[0][1] },
        { a.raw[0][0] * b.raw[1][0] + a.raw[1][0] * b.raw[1][1], a.raw[0][1] * b.raw[1][0] + a.raw[1][1] * b.raw[1][1] }
    }};
}

// mat3
inline Vec3 operator*(const Mat3& m, const Vec3& v) {
    return {
        m.raw[0][0] * v.x + m.raw[1][0] * v.y + m.raw[2][0] * v.z,
        m.raw[0][1] * v.x + m.raw[1][1] * v.y + m.raw[2][1] * v.z,
        m.raw[0][2] * v.x + m.raw[1][2] * v.y + m.raw[2][2] * v.z
    };
}

inline Vec2 operator*(const Mat3& m, const Vec2& v) {
    return {
        m.raw[0][0] * v.x + m.raw[1][0] * v.y + m.raw[2][0],
        m.raw[0][1] * v.x + m.raw[1][1] * v.y + m.raw[2][1]
    };
}

inline Mat3 operator*(const Mat3& a, const Mat3& b) {
    Mat3 result = MAT3_ZERO;
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            result.raw[col][row] = a.raw[0][row] * b.raw[col][0] +
                                   a.raw[1][row] * b.raw[col][1] +
                                   a.raw[2][row] * b.raw[col][2];
        }
    }
    return result;
}

// mat4
inline Vec4 operator*(const Mat4& m, const Vec4& v) {
    return {
        m.raw[0][0] * v.x + m.raw[1][0] * v.y + m.raw[2][0] * v.z + m.raw[3][0] * v.w,
        m.raw[0][1] * v.x + m.raw[1][1] * v.y + m.raw[2][1] * v.z + m.raw[3][1] * v.w,
        m.raw[0][2] * v.x + m.raw[1][2] * v.y + m.raw[2][2] * v.z + m.raw[3][2] * v.w,
        m.raw[0][3] * v.x + m.raw[1][3] * v.y + m.raw[2][3] * v.z + m.raw[3][3] * v.w
    };
}

inline Vec3 operator*(const Mat4& m, const Vec3& v) {
    return {
        m.raw[0][0] * v.x + m.raw[1][0] * v.y + m.raw[2][0] * v.z + m.raw[3][0],
        m.raw[0][1] * v.x + m.raw[1][1] * v.y + m.raw[2][1] * v.z + m.raw[3][1],
        m.raw[0][2] * v.x + m.raw[1][2] * v.y + m.raw[2][2] * v.z + m.raw[3][2]
    };
}

inline Mat4 operator*(const Mat4& a, const Mat4& b) {
    Mat4 result = MAT4_ZERO;
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            result.raw[col][row] = a.raw[0][row] * b.raw[col][0] +
                                   a.raw[1][row] * b.raw[col][1] +
                                   a.raw[2][row] * b.raw[col][2] +
                                   a.raw[3][row] * b.raw[col][3];
        }
    }
    return result;
}

inline Mat4& operator*=(Mat4& a, const Mat4& b) {
    a = a * b;
    return a;
}

// CONVERT
inline Vec3 vec3_from_vec2(Vec2 v, f32 z = 0) { return { v.x, v.y, z }; }
inline Vec4 vec4_from_vec3(Vec3 v, f32 w = 0) { return { v.x, v.y, v.z, w }; }
inline Vec2 vec2_xy(Vec3 v) { return { v.x, v.y }; }
inline Vec2 vec2_xz(Vec3 v) { return { v.x, v.z }; }
inline Vec2 vec2_yz(Vec3 v) { return { v.y, v.z }; }
inline Vec3 vec3_xyz(Vec4 v) { return { v.x, v.y, v.z }; }

// VEC FUNCTIONS
// vec2
inline f32 vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

inline f32 vec2_cross(Vec2 a, Vec2 b) {
    return a.x * b.y - a.y * b.x;
}

inline f32 vec2_sqr_length(Vec2 v) {
    return vec2_dot(v, v);
}

inline f32 vec2_length(Vec2 v) {
    return math_sqrt(vec2_sqr_length(v));
}

inline Vec2 vec2_normalize(Vec2 v) {
    f32 len = vec2_length(v);
    return len > MATH_EPSILON ? v / len : VEC2_ZERO;
}

inline Vec2 vec2_lerp(Vec2 a, Vec2 b, f32 t) {
    return a + (b - a) * t;
}

inline f32 vec2_distance(Vec2 a, Vec2 b) {
    return vec2_length(a - b);
}

inline Vec2 vec2_reflect(Vec2 v, Vec2 n) {
    return v - n * (2 * vec2_dot(v, n));
}

inline bool vec2_approximate(Vec2 a, Vec2 b, f32 epsilon = MATH_EPSILON) {
    return math_abs(a.x - b.x) <= epsilon && math_abs(a.y - b.y) <= epsilon;
}

// vec3
inline f32 vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 vec3_cross(Vec3 a, Vec3 b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline f32 vec3_sqr_length(Vec3 v) {
    return vec3_dot(v, v);
}

inline f32 vec3_length(Vec3 v) {
    return math_sqrt(vec3_sqr_length(v));
}

inline Vec3 vec3_normalize(Vec3 v) {
    f32 len = vec3_length(v);
    return len > MATH_EPSILON ? v / len : VEC3_ZERO;
}

inline Vec3 vec3_lerp(Vec3 a, Vec3 b, f32 t) {
    return a + (b - a) * t;
}

inline f32 vec3_distance(Vec3 a, Vec3 b) {
    return vec3_length(a - b);
}

inline Vec3 vec3_reflect(Vec3 v, Vec3 n) {
    return v - n * (2 * vec3_dot(v, n));
}

inline Vec3 vec3_project(Vec3 v, Vec3 target) {
    f32 sqr_len = vec3_sqr_length(target);
    if (sqr_len <= MATH_EPSILON) {
        return VEC3_ZERO;
    }
    return target * (vec3_dot(v, target) / sqr_len);
}

inline Vec3 vec3_project_plane(Vec3 v, Vec3 normal) {
    return v - vec3_project(v, normal);
}

inline bool vec3_approximate(Vec3 a, Vec3 b, f32 epsilon = MATH_EPSILON) {
    return math_abs(a.x - b.x) <= epsilon &&
           math_abs(a.y - b.y) <= epsilon &&
           math_abs(a.z - b.z) <= epsilon;
}

// QUAT FUNCTIONS
inline Quat quat_mul(Quat q1, Quat q2) {
    return {
        q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y,
        q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x,
        q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w,
        q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z
    };
}

inline Quat quat_conjugate(Quat q) {
    return { -q.x, -q.y, -q.z, q.w };
}

inline f32 quat_sqr_length(Quat q) {
    return q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
}

inline f32 quat_length(Quat q) {
    return math_sqrt(quat_sqr_length(q));
}

inline Quat quat_normalize(Quat q) {
    f32 len = quat_length(q);
    return len > MATH_EPSILON ? q / len : QUAT_IDENTITY;
}

// MAT FUNCTIONS
inline Mat4 mat4_transpose(const Mat4* m) {
    return Mat4{ .raw = {
        { m->raw[0][0], m->raw[1][0], m->raw[2][0], m->raw[3][0] },
        { m->raw[0][1], m->raw[1][1], m->raw[2][1], m->raw[3][1] },
        { m->raw[0][2], m->raw[1][2], m->raw[2][2], m->raw[3][2] },
        { m->raw[0][3], m->raw[1][3], m->raw[2][3], m->raw[3][3] }
    }};
}

inline Mat4 mat4_translate(Vec3 translation) {
    Mat4 m = MAT4_IDENTITY;
    m.raw[3][0] = translation.x;
    m.raw[3][1] = translation.y;
    m.raw[3][2] = translation.z;
    return m;
}

inline Mat4 mat4_rotation(Quat q) {
    f32 xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    f32 xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    f32 wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    return Mat4{ .raw = {
        { 1 - 2 * (yy + zz),     2 * (xy + wz),     2 * (xz - wy), 0 },
        {     2 * (xy - wz), 1 - 2 * (xx + zz),     2 * (yz + wx), 0 },
        {     2 * (xz + wy),     2 * (yz - wx), 1 - 2 * (xx + yy), 0 },
        {                 0,                 0,                 0, 1 }
    }};
}

inline Mat4 mat4_scale(Vec3 scale) {
    Mat4 m = MAT4_IDENTITY;
    m.raw[0][0] = scale.x;
    m.raw[1][1] = scale.y;
    m.raw[2][2] = scale.z;
    return m;
}

inline Mat4 mat4_perspective(f32 fov_radians, f32 aspect_ratio, f32 z_near, f32 z_far) {
    f32 tan_half_fov = tanf(fov_radians / 2);
    Mat4 m = MAT4_ZERO;
    m.raw[0][0] = 1 / (aspect_ratio * tan_half_fov);
    m.raw[1][1] = 1 / tan_half_fov;
    m.raw[2][2] = -(z_far + z_near) / (z_far - z_near);
    m.raw[2][3] = -1;
    m.raw[3][2] = -(2 * z_far * z_near) / (z_far - z_near);
    return m;
}

}
