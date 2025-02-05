#pragma once

#include <stdint.h>
#include <math.h>
#include <iostream>

template<typename T>
struct PTVector2
{
	T x, y;

	inline void operator=(const PTVector2<T>& a) { x = a.x; y = a.y; }
	inline void operator+=(const PTVector2<T>& a) { x += a.x; y += a.y; }
	inline void operator-=(const PTVector2<T>& a) { x -= a.x; y -= a.y; }
	inline void operator*=(const PTVector2<T>& a) { x *= a.x; y *= a.y; }
	inline void operator/=(const PTVector2<T>& a) { x /= a.x; y /= a.y; }
	inline void operator*=(const T f) { x *= f; y *= f; }
	inline void operator/=(const T f) { x /= f; y /= f; }
	inline PTVector2<T> operator -() { return PTVector2<T>{ -x, -y }; }
};

template<typename T>
inline PTVector2<T> operator+(const PTVector2<T>& a, const PTVector2<T>& b) { return PTVector2<T>{ a.x + b.x, a.y + b.y }; }
template<typename T>
inline PTVector2<T> operator-(const PTVector2<T>& a, const PTVector2<T>& b) { return PTVector2<T>{ a.x - b.x, a.y - b.y }; }
template<typename T>
inline PTVector2<T> operator*(const PTVector2<T>& a, const PTVector2<T>& b) { return PTVector2<T>{ a.x * b.x, a.y * b.y }; }
template<typename T>
inline PTVector2<T> operator/(const PTVector2<T>& a, const PTVector2<T>& b) { return PTVector2<T>{ a.x / b.x, a.y / b.y }; }
template<typename T>
inline PTVector2<T> operator*(const PTVector2<T>& a, const T f) { return PTVector2<T>{ a.x * f, a.y * f }; }
template<typename T>
inline PTVector2<T> operator/(const PTVector2<T>& a, const T f) { return PTVector2<T>{ a.x / f, a.y / f }; }
template<typename T>
inline bool operator==(const PTVector2<T>& a, const PTVector2<T>& b) { return (a.x == b.x) && (a.y == b.y); }

template<typename T>
inline T operator^(const PTVector2<T>& a, const PTVector2<T>& b) { return (a.x * b.x) + (a.y * b.y); }

template<typename T>
inline T sq_mag(const PTVector2<T>& a) { return a ^ a; }
template<typename T>
inline float mag(const PTVector2<T>& a) { return sqrt(sq_mag(a)); }
template<typename T>
inline PTVector2<T> norm(const PTVector2<T>& a) { return a / mag(a); }

template<typename T>
inline std::ostream& operator<<(std::ostream& stream, const PTVector2<T>& v) { return stream << '(' << v.x << ", " << v.y << ')'; }

typedef PTVector2<float> PTVector2f;
typedef PTVector2<int32_t> PTVector2i;
typedef PTVector2<uint32_t> PTVector2u;