#pragma once

#include <stdint.h>
#include <math.h>
#include <iostream>
#include <format>

template<typename T>
struct PTVector4
{
    T x, y, z, w;

    inline void operator=(const PTVector4<T>& a) { x = a.x; y = a.y; z = a.z; w = a.w; }
    inline void operator+=(const PTVector4<T>& a) { x += a.x; y += a.y; z += a.z; w += a.w; }
    inline void operator-=(const PTVector4<T>& a) { x -= a.x; y -= a.y; z -= a.z; w -= a.w; }
    inline void operator*=(const PTVector4<T>& a) { x *= a.x; y *= a.y; z *= a.z; w *= a.w; }
    inline void operator/=(const PTVector4<T>& a) { x /= a.x; y /= a.y; z /= a.z; w /= a.w; }
    inline void operator*=(const T f) { x *= f; y *= f; z *= f; w *= f; }
    inline void operator/=(const T f) { x /= f; y /= f; z /= f; w /= f; }
    inline PTVector4<T> operator -() { return PTVector4<T>{ -x, -y, -z, -w }; }
};

template<typename T>
inline PTVector4<T> operator+(const PTVector4<T>& a, const PTVector4<T>& b) { return PTVector4<T>{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w }; }
template<typename T>
inline PTVector4<T> operator-(const PTVector4<T>& a, const PTVector4<T>& b) { return PTVector4<T>{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w }; }
template<typename T>
inline PTVector4<T> operator*(const PTVector4<T>& a, const PTVector4<T>& b) { return PTVector4<T>{ a.x* b.x, a.y* b.y, a.z* b.z, a.w* b.w }; }
template<typename T>
inline PTVector4<T> operator/(const PTVector4<T>& a, const PTVector4<T>& b) { return PTVector4<T>{ a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w }; }
template<typename T>
inline PTVector4<T> operator*(const PTVector4<T>& a, const T f) { return PTVector4<T>{ a.x* f, a.y* f, a.z* f, a.w* f }; }
template<typename T>
inline PTVector4<T> operator/(const PTVector4<T>& a, const T f) { return PTVector4<T>{ a.x / f, a.y / f, a.z / f, a.w / f }; }
template<typename T>
inline bool operator==(const PTVector4<T>& a, const PTVector4<T>& b) { return (a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.w == b.w); }

template<typename T>
inline T operator^(const PTVector4<T>& a, const PTVector4<T>& b) { return (a.x * b.x) + (a.y * b.y) + (a.z * b.z) + (a.w * b.w); }
template<typename T>
inline T mag_sq(const PTVector4<T>& a) { return (a.x * a.x) + (a.y * a.y) + (a.z * a.z) + (a.w * a.w); }
template<typename T>
inline float mag(const PTVector4<T>& a) { return sqrt((a.x * a.x) + (a.y * a.y) + (a.z * a.z) + (a.w * a.w)); }
template<typename T>
inline PTVector4<T> norm(const PTVector4<T>& a) { return a / mag(a); }
template<typename T>
inline PTVector4<T> lerp(const PTVector4<T>& a, const PTVector4<T>& b, const T f) { PTVector4<T>{ a.x + ((b.x - a.x) * f), a.y + ((b.y - a.y) * f), a.z + ((b.z - a.z) * f), a.w + ((b.w - a.w) * f) }; }

template<typename T>
inline std::ostream& operator<<(std::ostream& stream, const PTVector4<T>& v) { return stream << '(' << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ')'; }

template<typename T>
inline std::string to_string(const PTVector4<T>& v) { return std::format("({:.3f},{:.3f},{:.3f},{:.3f})", v.x, v.y, v.z, v.w); }

typedef PTVector4<float> PTVector4f;
typedef PTVector4<int32_t> PTVector4i;
typedef PTVector4<uint32_t> PTVector4u;