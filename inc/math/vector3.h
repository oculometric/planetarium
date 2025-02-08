#pragma once

#include <stdint.h>
#include <math.h>
#include <iostream>
#include <format>

template<typename T>
struct PTVector3
{
    T x, y, z;

    inline void operator=(const PTVector3<T>& a) { x = a.x; y = a.y; z = a.z; }
    inline void operator+=(const PTVector3<T>& a) { x += a.x; y += a.y; z += a.z; }
    inline void operator-=(const PTVector3<T>& a) { x -= a.x; y -= a.y; z -= a.z; }
    inline void operator*=(const PTVector3<T>& a) { x *= a.x; y *= a.y; z *= a.z; }
    inline void operator/=(const PTVector3<T>& a) { x /= a.x; y /= a.y; z /= a.z; }
    inline void operator*=(const T f) { x *= f; y *= f; z *= f; }
    inline void operator/=(const T f) { x /= f; y /= f; z /= f; }
    inline PTVector3<T> operator -() { return PTVector3<T>{ -x, -y, -z }; }

    inline static constexpr PTVector3<T> right() { return PTVector3<T>{ 1, 0, 0 }; }
    inline static constexpr PTVector3<T> up() { return PTVector3<T>{ 0, 1, 0 }; }
    inline static constexpr PTVector3<T> forward() { return PTVector3<T>{ 0, 0, 1 }; }
};

template<typename T>
inline PTVector3<T> operator+(const PTVector3<T>& a, const PTVector3<T>& b) { return PTVector3<T>{ a.x + b.x, a.y + b.y, a.z + b.z }; }
template<typename T>
inline PTVector3<T> operator-(const PTVector3<T>& a, const PTVector3<T>& b) { return PTVector3<T>{ a.x - b.x, a.y - b.y, a.z - b.z }; }
template<typename T>
inline PTVector3<T> operator*(const PTVector3<T>& a, const PTVector3<T>& b) { return PTVector3<T>{ a.x * b.x, a.y * b.y, a.z * b.z }; }
template<typename T>
inline PTVector3<T> operator/(const PTVector3<T>& a, const PTVector3<T>& b) { return PTVector3<T>{ a.x / b.x, a.y / b.y, a.z / b.z }; }
template<typename T>
inline PTVector3<T> operator*(const PTVector3<T>& a, const float f) { return PTVector3<T>{ a.x * f, a.y * f, a.z * f }; }
template<typename T>
inline PTVector3<T> operator/(const PTVector3<T>& a, const float f) { return PTVector3<T>{ a.x / f, a.y / f, a.z / f }; }
template<typename T>
inline bool operator==(const PTVector3<T>& a, const PTVector3<T>& b) { return (a.x == b.x) && (a.y == b.y) && (a.z == b.z); }

template<typename T>
inline T operator^(const PTVector3<T>& a, const PTVector3<T>& b) { return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }
template<typename T>
inline PTVector3<T> operator%(const PTVector3<T>& a, const PTVector3<T>& b) { return PTVector3<T>{ (a.y * b.z) - (a.z * b.y), (a.z * b.x) - (a.x * b.z), (a.x * b.y) - (a.y * b.x) }; }

template<typename T>
inline T sq_mag(const PTVector3<T>& a) { return a ^ a; }
template<typename T>
inline float mag(const PTVector3<T>& a) { return sqrt(sq_mag(a)); }
template<typename T>
inline PTVector3<T> norm(const PTVector3<T>& a) { return a / mag(a); }
template<typename T>
inline PTVector3<T> lerp(const PTVector3<T>& a, const PTVector3<T>& b, const T f) { PTVector3<T>{ a.x + ((b.x - a.x) * f), a.y + ((b.y - a.y) * f), a.z + ((b.z - a.z) * f) }; }

template<typename T>
inline std::ostream& operator<<(std::ostream& stream, const PTVector3<T>& v) { return stream << '(' << v.x << ", " << v.y << ", " << v.z << ')'; }

template<typename T>
inline std::string to_string(const PTVector3<T>& v) { return std::format("({:.3f},{:.3f},{:.3f})", v.x, v.y, v.z); }

typedef PTVector3<float> PTVector3f;
typedef PTVector3<int32_t> PTVector3i;
typedef PTVector3<uint32_t> PTVector3u;