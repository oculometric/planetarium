#pragma once

#include "vector4.h"
#include "vector3.h"
#include "matrix4.h"
#include "debug.h"
#include <format>

struct PTQuaternion
{
    float i, j, k, r;

    inline PTQuaternion() : i(0), j(0), k(0), r(1) { }
    inline PTQuaternion(float _i, float _j, float _k, float _r) : i(_i), j(_j), k(_k), r(_r) { }
    inline PTQuaternion(const PTVector4f& v) { i = v.x; j = v.y; k = v.z; r = v.w; }
    inline PTQuaternion(const PTMatrix4f& m)
    {
        // FIXME: make this resilient to div/0 errors [https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/]
        r = sqrt(1 + m.x_0 + m.y_1 + m.z_2) / 2.0f;
        float w4 = 4.0f * r;
        i = (m.y_2 - m.z_1) / w4;
        j = (m.z_0 - m.x_2) / w4;
        k = (m.x_1 - m.y_0) / w4;
    }
    inline PTQuaternion(const PTVector3f& axis, const float angle)
    {
        float rad = -angle * ((float)M_PI / 180.0f) / 2.0f;
        i = axis.x * sin(rad);
        j = axis.y * sin(rad);
        k = axis.z * sin(rad);
        r = cos(rad);
        if (j > 5.0f)
            debugLog("nan!");
    }
    inline operator PTVector4f() const { return PTVector4f{ i, j, k, r }; }
    inline void operator=(const PTQuaternion& a) { i = a.i; j = a.j; k = a.k; r = a.r; }
    inline void operator+=(PTQuaternion q) { r += q.r; i += q.i; j += q.j; k += q.k; }
    inline void operator-=(PTQuaternion q) { r -= q.r; i -= q.i; j -= q.j; k -= q.k; }
    void operator*=(float s) { r *= s; i *= s; j *= s; k *= s; }
    void operator/=(float s) { r /= s; i /= s; j /= s; k /= s; }
    inline PTQuaternion operator~() const { return PTQuaternion{ -i, -j, -k, r }; }
};

inline PTQuaternion operator+(const PTQuaternion& a, const PTQuaternion& b) { return PTQuaternion{ a.i + b.i, a.j + b.j, a.k + b.k, a.r + b.r }; }
inline PTQuaternion operator-(const PTQuaternion& a, const PTQuaternion& b) { return PTQuaternion{ a.i - b.i, a.j - b.j, a.k - b.k, a.r - b.r }; }
inline PTQuaternion operator*(const PTQuaternion& a, const PTQuaternion& b)
{
    PTQuaternion quat
    {
        a.r * b.i + a.i * b.r + a.j * b.k - a.k * b.j,
        a.r * b.j + a.j * b.r + a.k * b.i - a.i * b.k,
        a.r * b.k + a.k * b.r + a.i * b.j - a.j * b.i,
        a.r * b.r - a.i * b.i - a.j * b.j - a.k * b.k
    };

    return quat;
}
inline PTQuaternion operator*(const PTQuaternion& a, float s) { return PTQuaternion{ a.i * s, a.j * s, a.k * s, a.r * s }; }
inline PTQuaternion operator/(const PTQuaternion& a, float s) { return PTQuaternion{ a.i / s, a.j / s, a.k / s, a.r / s }; }
inline PTQuaternion operator*(const PTQuaternion& a, const PTVector3f& b)
{
    PTQuaternion quat
    {
        a.r * b.x + a.j * b.z - a.k * b.y,
        a.r * b.y + a.k * b.x - a.i * b.z,
        a.r * b.z + a.i * b.y - a.j * b.x,
      -(a.i * b.x + a.j * b.y + a.k * b.z)
    };

    return quat;
}

inline float mag(const PTQuaternion& a) { return sqrt(a.r * a.r + a.i * a.i + a.j * a.j + a.k * a.k); }
inline PTVector3f vec(const PTQuaternion& a) { return PTVector3f{ a.i, a.j, a.k }; }
inline float scale(const PTQuaternion& a) { return a.r; }
inline float angle(const PTQuaternion& a) { return (2.0f * acos(a.r)) * (180.0f / (float)M_PI); }
inline PTVector3f axis(const PTQuaternion& a)
{
    PTVector3f v = vec(a);
    float m = mag(v);

    if (m <= 0.0001f)
        return PTVector3f::forward();
    else
        return v / m;
}
inline PTQuaternion rotate(const PTQuaternion& a, const PTQuaternion& b) { return a * b * (~a); }
inline PTVector3f rotate(const PTQuaternion& a, const PTVector3f& b) { return vec(a * b * (~a)); }
inline PTMatrix4f mat(const PTQuaternion& a)
{
    PTVector3f r = rotate(a, PTVector3f::right());
    PTVector3f u = rotate(a, PTVector3f::up());
    PTVector3f f = rotate(a, PTVector3f::forward());

    return PTMatrix4f
    {
        r.x, u.x, f.x, 0,
        r.y, u.y, f.y, 0,
        r.z, u.z, f.z, 0,
        0,   0,   0,   1
    };
}

inline std::string to_string(const PTQuaternion& v) { return std::format("({:.3f}i,{:.3f}j,{:.3f}k,{:.3f}r)", v.i, v.j, v.k, v.r); }

// TODO: quat->euler and euler->quat