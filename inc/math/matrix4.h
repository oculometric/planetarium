#pragma once

#include <stdint.h>
#include "vector4.h"

// forward declarations

template <typename T>
struct PTMatrix4;

template <typename T>
inline T det(const PTMatrix4<T>& a);
template <typename T>
inline PTMatrix4<T> cofact(const PTMatrix4<T>& a);
template <typename T>
inline PTMatrix4<T> adj(const PTMatrix4<T>& a);

/**
    * structure which represents a 4x4 matrix
    * with components:
    * [ x_0, y_0, z_0, w_0 ]
    * [ x_1, y_1, z_1, w_1 ]
    * [ x_2, y_2, z_2, w_2 ]
    * [ x_3, y_3, z_3, w_3 ]
    *
    * elements are indexed left to right, top to bottom
    *
    * the `-` operator can be applied to find the transpose of the matrix
    * the `~` operator can be applied to find the inverse of the matrix
    * **/
template <typename T>
struct PTMatrix4
{
    T x_0, y_0, z_0, w_0;
    T x_1, y_1, z_1, w_1;
    T x_2, y_2, z_2, w_2;
    T x_3, y_3, z_3, w_3;

    constexpr PTMatrix4(const PTMatrix4<T>& o) : x_0(o.x_0), y_0(o.y_0), z_0(o.z_0), w_0(o.w_0), x_1(o.x_1), y_1(o.y_1), z_1(o.z_1), w_1(o.w_1), x_2(o.x_2), y_2(o.y_2), z_2(o.z_2), w_2(o.w_2), x_3(o.x_3), y_3(o.y_3), z_3(o.z_3), w_3(o.w_3) { }
    constexpr PTMatrix4(const T _x_0, const T _y_0, const T _z_0, const T _w_0, const T _x_1, const T _y_1, const T _z_1, const T _w_1, const T _x_2, const T _y_2, const T _z_2, const T _w_2, const T _x_3, const T _y_3, const T _z_3, const T _w_3) : x_0(_x_0), y_0(_y_0), z_0(_z_0), w_0(_w_0), x_1(_x_1), y_1(_y_1), z_1(_z_1), w_1(_w_1), x_2(_x_2), y_2(_y_2), z_2(_z_2), w_2(_w_2), x_3(_x_3), y_3(_y_3), z_3(_z_3), w_3(_w_3) { }
    constexpr PTMatrix4() : x_0(1), y_0(0), z_0(0), w_0(0), x_1(0), y_1(1), z_1(0), w_1(0), x_2(0), y_2(0), z_2(1), w_2(0), x_3(0), y_3(0), z_3(0), w_3(1) { }

    inline PTVector4<T> col0() const { return PTVector4<T>{ x_0, x_1, x_2, x_3 }; }
    inline PTVector4<T> col1() const { return PTVector4<T>{ y_0, y_1, y_2, y_3 }; }
    inline PTVector4<T> col2() const { return PTVector4<T>{ z_0, z_1, z_2, z_3 }; }
    inline PTVector4<T> col3() const { return PTVector4<T>{ w_0, w_1, w_2, w_3 }; }
    inline PTVector4<T> row0() const { return PTVector4<T>{ x_0, y_0, z_0, w_0 }; }
    inline PTVector4<T> row1() const { return PTVector4<T>{ x_1, y_1, z_1, w_1 }; }
    inline PTVector4<T> row2() const { return PTVector4<T>{ x_2, y_2, z_2, w_2 }; }
    inline PTVector4<T> row3() const { return PTVector4<T>{ x_3, y_3, z_3, w_3 }; }

    inline void operator=(const PTMatrix4<T>& a) { x_0 = a.x_0; y_0 = a.y_0; z_0 = a.z_0; w_0 = a.w_0; x_1 = a.x_1; y_1 = a.y_1; z_1 = a.z_1; w_1 = a.w_1; x_2 = a.x_2; y_2 = a.y_2; z_2 = a.z_2; w_2 = a.w_2; x_3 = a.x_3; y_3 = a.y_3; z_3 = a.z_3; w_3 = a.w_3; }
    inline void operator+=(const PTMatrix4<T>& a) { x_0 += a.x_0; y_0 += a.y_0; z_0 += a.z_0; w_0 += a.w_0; x_1 += a.x_1; y_1 += a.y_1; z_1 += a.z_1; w_1 += a.w_1; x_2 += a.x_2; y_2 += a.y_2; z_2 += a.z_2; w_2 += a.w_2; x_3 += a.x_3; y_3 += a.y_3; z_3 += a.z_3; w_3 += a.w_3; }
    inline void operator-=(const PTMatrix4<T>& a) { x_0 -= a.x_0; y_0 -= a.y_0; z_0 -= a.z_0; w_0 -= a.w_0; x_1 -= a.x_1; y_1 -= a.y_1; z_1 -= a.z_1; w_1 -= a.w_1; x_2 -= a.x_2; y_2 -= a.y_2; z_2 -= a.z_2; w_2 -= a.w_2; x_3 -= a.x_3; y_3 -= a.y_3; z_3 -= a.z_3; w_3 -= a.w_3; }
    inline void operator*=(const PTMatrix4<T>& a)
    {
        PTVector4<T> r0 = row0();
        PTVector4<T> r1 = row1();
        PTVector4<T> r2 = row2();
        PTVector4<T> r3 = row3();
        PTVector4<T> c0 = a.col0();
        PTVector4<T> c1 = a.col1();
        PTVector4<T> c2 = a.col2();
        PTVector4<T> c3 = a.col3();
        x_0 = r0 ^ c0; y_0 = r0 ^ c1; z_0 = r0 ^ c2; w_0 = r0 ^ c3;
        x_1 = r1 ^ c0; y_1 = r1 ^ c1; z_1 = r1 ^ c2; w_1 = r1 ^ c3;
        x_2 = r2 ^ c0; y_2 = r2 ^ c1; z_2 = r2 ^ c2; w_2 = r2 ^ c3;
        x_3 = r3 ^ c0; y_3 = r3 ^ c1; z_3 = r3 ^ c2; w_3 = r3 ^ c3;
    }
    inline void operator*=(const T a) { x_0 *= a; y_0 *= a; z_0 *= a; w_0 *= a; x_1 *= a; y_1 *= a; z_1 *= a; w_1 *= a; x_2 *= a; y_2 *= a; z_2 *= a; w_2 *= a; x_3 *= a; y_3 *= a; z_3 *= a; w_3 *= a; }
    inline void operator/=(const T a) { x_0 /= a; y_0 /= a; z_0 /= a; w_0 /= a; x_1 /= a; y_1 /= a; z_1 /= a; w_1 /= a; x_2 /= a; y_2 /= a; z_2 /= a; w_2 /= a; x_3 /= a; y_3 /= a; z_3 /= a; w_3 /= a; }
    inline PTMatrix4<T> operator-() const { return PTMatrix4<T>{ x_0, x_1, x_2, x_3, y_0, y_1, y_2, y_3, z_0, z_1, z_2, z_3, w_0, w_1, w_2, w_3 }; }
    inline PTMatrix4<T> operator~() const { return adj(*this) / det(*this); }

    inline void getColumnMajor(T* arr)
    {
        arr[0] = x_0;
        arr[1] = x_1;
        arr[2] = x_2;
        arr[3] = x_3;
        arr[4] = y_0;
        arr[5] = y_1;
        arr[6] = y_2;
        arr[7] = y_3;
        arr[8] = z_0;
        arr[9] = z_1;
        arr[10] = z_2;
        arr[11] = z_3;
        arr[12] = w_0;
        arr[13] = w_1;
        arr[14] = w_2;
        arr[15] = w_3;
    }

    inline void getRowMajor(T* arr)
    {
        arr[0] = x_0;
        arr[1] = y_0;
        arr[2] = z_0;
        arr[3] = w_0;
        arr[4] = x_1;
        arr[5] = y_1;
        arr[6] = z_1;
        arr[7] = w_1;
        arr[8] = x_2;
        arr[9] = y_2;
        arr[10] = z_2;
        arr[11] = w_2;
        arr[12] = x_3;
        arr[13] = y_3;
        arr[14] = z_3;
        arr[15] = w_3;
    }
};

template <typename T>
inline PTMatrix4<T> operator+(const PTMatrix4<T>& a, const PTMatrix4<T>& b)
{
    return PTMatrix4<T>
    {
        a.x_0 + b.x_0, a.y_0 + b.y_0, a.z_0 + b.z_0, a.w_0 + b.w_0,
            a.x_1 + b.x_1, a.y_1 + b.y_1, a.z_1 + b.z_1, a.w_1 + b.w_1,
            a.x_2 + b.x_2, a.y_2 + b.y_2, a.z_2 + b.z_2, a.w_2 + b.w_2,
            a.x_3 + b.x_3, a.y_3 + b.y_3, a.z_3 + b.z_3, a.w_3 + b.w_3
    };
}

template <typename T>
inline PTMatrix4<T> operator-(const PTMatrix4<T>& a, const PTMatrix4<T>& b)
{
    return PTMatrix4<T>
    {
        a.x_0 - b.x_0, a.y_0 - b.y_0, a.z_0 - b.z_0, a.w_0 - b.w_0,
            a.x_1 - b.x_1, a.y_1 - b.y_1, a.z_1 - b.z_1, a.w_1 - b.w_1,
            a.x_2 - b.x_2, a.y_2 - b.y_2, a.z_2 - b.z_2, a.w_2 - b.w_2,
            a.x_3 - b.x_3, a.y_3 - b.y_3, a.z_3 - b.z_3, a.w_3 - b.w_3
    };
}

template <typename T>
inline PTMatrix4<T> operator*(const PTMatrix4<T>& a, const PTMatrix4<T>& b)
{
    PTVector4<T> r0 = a.row0();
    PTVector4<T> r1 = a.row1();
    PTVector4<T> r2 = a.row2();
    PTVector4<T> r3 = a.row3();
    PTVector4<T> c0 = b.col0();
    PTVector4<T> c1 = b.col1();
    PTVector4<T> c2 = b.col2();
    PTVector4<T> c3 = b.col3();
    return PTMatrix4<T>
    {
        r0^ c0, r0^ c1, r0^ c2, r0^ c3,
            r1^ c0, r1^ c1, r1^ c2, r1^ c3,
            r2^ c0, r2^ c1, r2^ c2, r2^ c3,
            r3^ c0, r3^ c1, r3^ c2, r3^ c3
    };
}

template <typename T, typename U>
inline PTVector4<U> operator*(const PTMatrix4<T>& a, const PTVector4<U>& b)
{
    return PTVector4<U>{ a.row0() ^ b, a.row1() ^ b, a.row2() ^ b, a.row3() ^ b };
}

template <typename T>
inline PTMatrix4<T> operator*(const PTMatrix4<T>& a, const T b)
{
    return PTMatrix4<T>
    {
        a.x_0* b, a.y_0* b, a.z_0* b, a.w_0* b,
            a.x_1* b, a.y_1* b, a.z_1* b, a.w_1* b,
            a.x_2* b, a.y_2* b, a.z_2* b, a.w_2* b,
            a.x_3* b, a.y_3* b, a.z_3* b, a.w_3* b
    };
}

template <typename T>
inline PTMatrix4<T> operator/(const PTMatrix4<T>& a, const T b)
{
    return PTMatrix4<T>
    {
        a.x_0 / b, a.y_0 / b, a.z_0 / b, a.w_0 / b,
            a.x_1 / b, a.y_1 / b, a.z_1 / b, a.w_1 / b,
            a.x_2 / b, a.y_2 / b, a.z_2 / b, a.w_2 / b,
            a.x_3 / b, a.y_3 / b, a.z_3 / b, a.w_3 / b
    };
}

/**
    * calculate the minor of a matrix
    *
    * - by taking the determinant of the matrix when the row and cPTumn
    * containing the specified element are discarded.
    *
    * elements are ordered:
    *
    * [ 00, 01, 02, 03 ]
    * [ 04, 05, 06, 07 ]
    * [ 08, 09, 10, 11 ]
    * [ 12, 13, 14, 15 ]
    *
    * @param a matrix on which to operate
    * @param e index of the element for which to find the minor matrix. see above
    *
    * @return determinant of the resulting minor matrix
    * **/
template <typename T>
inline T minor(const PTMatrix4<T>& a, const uint8_t e)
{
    switch (e)
    {
    case  0: return ((a.y_1 * ((a.z_2 * a.w_3) - (a.w_2 * a.z_3))) - (a.z_1 * ((a.y_2 * a.w_3) - (a.w_2 * a.y_3)))) + (a.w_1 * ((a.y_2 * a.z_3) - (a.z_2 * a.y_3)));
    case  1: return ((a.x_1 * ((a.z_2 * a.w_3) - (a.w_2 * a.z_3))) - (a.z_1 * ((a.x_2 * a.w_3) - (a.w_2 * a.x_3)))) + (a.w_1 * ((a.x_2 * a.z_3) - (a.z_2 * a.x_3)));
    case  2: return ((a.x_1 * ((a.y_2 * a.w_3) - (a.w_2 * a.y_3))) - (a.y_1 * ((a.x_2 * a.w_3) - (a.w_2 * a.x_3)))) + (a.w_1 * ((a.x_2 * a.y_3) - (a.y_2 * a.x_3)));
    case  3: return ((a.x_1 * ((a.y_2 * a.z_3) - (a.z_2 * a.y_3))) - (a.y_1 * ((a.x_2 * a.z_3) - (a.z_2 * a.x_3)))) + (a.z_1 * ((a.x_2 * a.y_3) - (a.y_2 * a.x_3)));
    case  4: return ((a.y_0 * ((a.z_2 * a.w_3) - (a.w_2 * a.z_3))) - (a.z_0 * ((a.y_2 * a.w_3) - (a.w_2 * a.y_3)))) + (a.w_0 * ((a.y_2 * a.z_3) - (a.z_2 * a.y_3)));
    case  5: return ((a.x_0 * ((a.z_2 * a.w_3) - (a.w_2 * a.z_3))) - (a.z_0 * ((a.x_2 * a.w_3) - (a.w_2 * a.x_3)))) + (a.w_0 * ((a.x_2 * a.z_3) - (a.z_2 * a.x_3)));
    case  6: return ((a.x_0 * ((a.y_2 * a.w_3) - (a.w_2 * a.y_3))) - (a.y_0 * ((a.x_2 * a.w_3) - (a.w_2 * a.x_3)))) + (a.w_0 * ((a.x_2 * a.y_3) - (a.y_2 * a.x_3)));
    case  7: return ((a.x_0 * ((a.y_2 * a.z_3) - (a.z_2 * a.y_3))) - (a.y_0 * ((a.x_2 * a.z_3) - (a.z_2 * a.x_3)))) + (a.z_0 * ((a.x_2 * a.y_3) - (a.y_2 * a.x_3)));
    case  8: return ((a.y_0 * ((a.z_1 * a.w_3) - (a.w_1 * a.z_3))) - (a.z_0 * ((a.y_1 * a.w_3) - (a.w_1 * a.y_3)))) + (a.w_0 * ((a.y_1 * a.z_3) - (a.z_1 * a.y_3)));
    case  9: return ((a.x_0 * ((a.z_1 * a.w_3) - (a.w_1 * a.z_3))) - (a.z_0 * ((a.x_1 * a.w_3) - (a.w_1 * a.x_3)))) + (a.w_0 * ((a.x_1 * a.z_3) - (a.z_1 * a.x_3)));
    case 10: return ((a.x_0 * ((a.y_1 * a.w_3) - (a.w_1 * a.y_3))) - (a.y_0 * ((a.x_1 * a.w_3) - (a.w_1 * a.x_3)))) + (a.w_0 * ((a.x_1 * a.y_3) - (a.y_1 * a.x_3)));
    case 11: return ((a.x_0 * ((a.y_1 * a.z_3) - (a.z_1 * a.y_3))) - (a.y_0 * ((a.x_1 * a.z_3) - (a.z_1 * a.x_3)))) + (a.z_0 * ((a.x_1 * a.y_3) - (a.y_1 * a.x_3)));
    case 12: return ((a.y_0 * ((a.z_1 * a.w_2) - (a.w_1 * a.z_2))) - (a.z_0 * ((a.y_1 * a.w_2) - (a.w_1 * a.y_2)))) + (a.w_0 * ((a.y_1 * a.z_2) - (a.z_1 * a.y_2)));
    case 13: return ((a.x_0 * ((a.z_1 * a.w_2) - (a.w_1 * a.z_2))) - (a.z_0 * ((a.x_1 * a.w_2) - (a.w_1 * a.x_2)))) + (a.w_0 * ((a.x_1 * a.z_2) - (a.z_1 * a.x_2)));
    case 14: return ((a.x_0 * ((a.y_1 * a.w_2) - (a.w_1 * a.y_2))) - (a.y_0 * ((a.x_1 * a.w_2) - (a.w_1 * a.x_2)))) + (a.w_0 * ((a.x_1 * a.y_2) - (a.y_1 * a.x_2)));
    case 15: return ((a.x_0 * ((a.y_1 * a.z_2) - (a.z_1 * a.y_2))) - (a.y_0 * ((a.x_1 * a.z_2) - (a.z_1 * a.x_2)))) + (a.z_0 * ((a.x_1 * a.y_2) - (a.y_1 * a.x_2)));
    default: return 0;
    }
}

/**
    * calculate determinant of a matrix
    *
    * - by multiplying three elements by their minors and alternately adding
    * and subtracting them together
    *
    * @param a matrix on which to operate
    *
    * @return determinant of the matrix
    * **/
template <typename T>
inline T det(const PTMatrix4<T>& a) { return ((a.x_0 * minor(a, 0)) - (a.y_0 * minor(a, 1))) + ((a.z_0 * minor(a, 2)) - (a.w_0 * minor(a, 3))); }

/**
    * calculate matrix of cofactors from a matrix
    *
    * - by finding the matrix of minors (each element replaced with
    * its minor), then multiplying by a checkered mask of alternating 1, -1
    *
    * @param a matrix on which to operate
    *
    * @return matrix of cofactors
    * **/
template <typename T>
inline PTMatrix4<T> cofact(const PTMatrix4<T>& a) { return PTMatrix4<T>{ minor(a, 0), -minor(a, 1), minor(a, 2), -minor(a, 3), -minor(a, 4), minor(a, 5), -minor(a, 6), minor(a, 7), minor(a, 8), -minor(a, 9), minor(a, 10), -minor(a, 11), -minor(a, 12), minor(a, 13), -minor(a, 14), minor(a, 15) }; }

/**
 * calculate adjoint of a matrix
 *
 * - by finding the transpose of the matrix of cofactors of
 * the input matrix
 *
 * @param a matrix on which to operate
 *
 * @return adjoint matrix of the input
 * **/
template <typename T>
inline PTMatrix4<T> adj(const PTMatrix4<T>& a) { return -cofact(a); }

typedef PTMatrix4<float> PTMatrix4f;

/**
 * convert a quaternion (i.e. a vector4) to a rotation matrix. the input 
 * quaternion must be normalised, and formatted as (x, y, z, r). sorry mathematicians
 * 
 * @param quat quaternion to use
 * 
 * @return matrix which represents the rotation described by the quaternion
 * **/
inline PTMatrix4f toMatrix(const PTVector4f& quat)
{
    float qi = quat.x; float qi2 = qi * qi;
    float qj = quat.y; float qj2 = qj * qj;
    float qk = quat.z; float qk2 = qk * qk;
    float qr = quat.w;

    PTMatrix4f rotation = PTMatrix4f
    {
        1 - (2 * (qj2 + qk2)),         2 * ((qi * qj) - (qk * qr)),   2 * ((qi * qk) + (qj * qr)),   0,
        2 * ((qi * qj) + (qk * qr)),   1 - (2 * (qi2 + qk2)),         2 * ((qj * qk) - (qi * qr)),   0,
        2 * ((qi * qk) - (qj * qr)),   2 * ((qj * qk) + (qi * qr)),   1 - (2 * (qi2 + qj2)),         0,
        0,                             0,                             0,                             1
    };

    return rotation;
}
