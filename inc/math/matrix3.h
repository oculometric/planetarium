#pragma once

#include "vector3.h"

// forward declarations

template <typename T>
struct PTMatrix3;

template <typename T>
inline T det(const PTMatrix3<T>& a);
template <typename T>
inline PTMatrix3<T> cofact(const PTMatrix3<T>& a);
template <typename T>
inline PTMatrix3<T> adj(const PTMatrix3<T>& a);

/**
 * structure which represents a 3x3 matrix
 * with components:
 * [ x_0, y_0, z_0 ]
 * [ x_1, y_1, z_1 ]
 * [ x_2, y_2, z_2 ]
 * 
 * elements are indexed left to right, top to bottom
 * 
 * the `-` operator can be applied to find the transpose of the matrix
 * the `~` operator can be applied to find the inverse of the matrix
 * **/
template <typename T>
struct PTMatrix3
{
    T x_0, y_0, z_0;
    T x_1, y_1, z_1;
    T x_2, y_2, z_2;

    constexpr PTMatrix3(const PTMatrix3<T>& o) : x_0(o.x_0), y_0(o.y_0), z_0(o.z_0), x_1(o.x_1), y_1(o.y_1), z_1(o.z_1), x_2(o.x_2), y_2(o.y_2), z_2(o.z_2) { }
    constexpr PTMatrix3(const T _x_0, const T _y_0, const T _z_0, const T _x_1, const T _y_1, const T _z_1, const T _x_2, const T _y_2, const T _z_2) : x_0(_x_0), y_0(_y_0), z_0(_z_0), x_1(_x_1), y_1(_y_1), z_1(_z_1), x_2(_x_2), y_2(_y_2), z_2(_z_2) { }
    constexpr PTMatrix3(): x_0(0), y_0(0), z_0(0), x_1(0), y_1(0), z_1(0), x_2(0), y_2(0), z_2(0) { }

    inline PTVector3<T> col0() const { return PTVector3<T>{ x_0,x_1,x_2 }; }
    inline PTVector3<T> col1() const { return PTVector3<T>{ y_0,y_1,y_2 }; }
    inline PTVector3<T> col2() const { return PTVector3<T>{ z_0,z_1,z_2 }; }
    inline PTVector3<T> row0() const { return PTVector3<T>{ x_0,y_0,z_0 }; }
    inline PTVector3<T> row1() const { return PTVector3<T>{ x_1,y_1,z_1 }; }
    inline PTVector3<T> row2() const { return PTVector3<T>{ x_2,y_2,z_2 }; }

    inline void operator=(const PTMatrix3<T>& a) { x_0 = a.x_0; y_0 = a.y_0; z_0 = a.z_0; x_1 = a.x_1; y_1 = a.y_1; z_1 = a.z_1; x_2 = a.x_2; y_2 = a.y_2; z_2 = a.z_2; }
    inline void operator+=(const PTMatrix3<T>& a) { x_0 += a.x_0; y_0 += a.y_0; z_0 += a.z_0; x_1 += a.x_1; y_1 += a.y_1; z_1 += a.z_1; x_2 += a.x_2; y_2 += a.y_2; z_2 += a.z_2; }
    inline void operator-=(const PTMatrix3<T>& a) { x_0 -= a.x_0; y_0 -= a.y_0; z_0 -= a.z_0; x_1 -= a.x_1; y_1 -= a.y_1; z_1 -= a.z_1; x_2 -= a.x_2; y_2 -= a.y_2; z_2 -= a.z_2; }
    inline void operator*=(const PTMatrix3<T>& a)
    {
        PTVector3<T> r0 = row0();
        PTVector3<T> r1 = row1();
        PTVector3<T> r2 = row2();
        PTVector3<T> c0 = a.col0();
        PTVector3<T> c1 = a.col1();
        PTVector3<T> c2 = a.col2();
        x_0 = r0 ^ c0; y_0 = r0 ^ c1; z_0 = r0 ^ c2;
        x_1 = r1 ^ c0; y_1 = r1 ^ c1; z_1 = r1 ^ c2;
        x_2 = r2 ^ c0; y_2 = r2 ^ c1; z_2 = r2 ^ c2;
    }
    inline void operator*=(const T a) { x_0 *= a; y_0 *= a; z_0 *= a; x_1 *= a; y_1 *= a; z_1 *= a; x_2 *= a; y_2 *= a; z_2 *= a; }
    inline void operator/=(const T a) { x_0 /= a; y_0 /= a; z_0 /= a; x_1 /= a; y_1 /= a; z_1 /= a; x_2 /= a; y_2 /= a; z_2 /= a; }
    inline PTMatrix3<T> operator-() const { return PTMatrix3<T>{ x_0, x_1, x_2, y_0, y_1, y_2, z_0, z_1, z_2 }; }
    inline PTMatrix3<T> operator~() const { return adj(*this) / det(*this); }
};

template <typename T>
inline PTMatrix3<T> operator+(const PTMatrix3<T>& a, const PTMatrix3<T>& b)
{
    return PTMatrix3<T>
    {
        a.x_0 + b.x_0, a.y_0 + b.y_0, a.z_0 + b.z_0,
        a.x_1 + b.x_1, a.y_1 + b.y_1, a.z_1 + b.z_1,
        a.x_2 + b.x_2, a.y_2 + b.y_2, a.z_2 + b.z_2
    };
}

template <typename T>
inline PTMatrix3<T> operator-(const PTMatrix3<T>& a, const PTMatrix3<T>& b)
{
    return PTMatrix3<T>
    {
        a.x_0 - b.x_0, a.y_0 - b.y_0, a.z_0 - b.z_0,
        a.x_1 - b.x_1, a.y_1 - b.y_1, a.z_1 - b.z_1,
        a.x_2 - b.x_2, a.y_2 - b.y_2, a.z_2 - b.z_2
    };
}

template <typename T>
inline PTMatrix3<T> operator*(const PTMatrix3<T>& a, const PTMatrix3<T>& b)
{
    PTVector3<T> r0 = a.row0();
    PTVector3<T> r1 = a.row1();
    PTVector3<T> r2 = a.row2();
    PTVector3<T> c0 = b.col0();
    PTVector3<T> c1 = b.col1();
    PTVector3<T> c2 = b.col2();
    return PTMatrix3<T>
    {
        r0 ^ c0, r0 ^ c1, r0 ^ c2,
        r1 ^ c0, r1 ^ c1, r1 ^ c2,
        r2 ^ c0, r2 ^ c1, r2 ^ c2
    };
}

template <typename T, typename U>
inline PTVector3<U> operator*(const PTMatrix3<T>& a, const PTVector3<U>& b)
{
    return PTVector3<U>{ a.row0() ^ b, a.row1() ^ b, a.row2() ^ b };
}

template <typename T>
inline PTMatrix3<T> operator*(const PTMatrix3<T>& a, const T b)
{
    return PTMatrix3<T>
    {
        a.x_0 * b, a.y_0 * b, a.z_0 * b,
        a.x_1 * b, a.y_1 * b, a.z_1 * b,
        a.x_2 * b, a.y_2 * b, a.z_2 * b
    };
}

template <typename T>
inline PTMatrix3<T> operator/(const PTMatrix3<T>& a, const T b)
{
    return PTMatrix3<T>
    {
        a.x_0 / b, a.y_0 / b, a.z_0 / b,
        a.x_1 / b, a.y_1 / b, a.z_1 / b,
        a.x_2 / b, a.y_2 / b, a.z_2 / b
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
 * [ 0, 1, 2 ]
 * [ 3, 4, 5 ]
 * [ 6, 7, 8 ]
 * 
 * @param a matrix on which to operate
 * @param e index of the element for which to find the minor matrix. see above
 * 
 * @return determinant of the resulting minor matrix
 * **/
template <typename T>
inline T minor(const PTMatrix3<T>& a, const uint8_t e)
{
    switch (e)
    {
        case 0: return (a.y_1 * a.z_2) - (a.y_2 * a.z_1);
        case 1: return (a.x_1 * a.z_2) - (a.x_2 * a.z_1);
        case 2: return (a.x_1 * a.y_2) - (a.y_1 * a.x_2);
        case 3: return (a.y_0 * a.z_2) - (a.z_0 * a.y_2);
        case 4: return (a.x_0 * a.z_2) - (a.z_0 * a.x_2);
        case 5: return (a.x_0 * a.y_2) - (a.y_0 * a.x_2);
        case 6: return (a.y_0 * a.z_1) - (a.y_1 * a.z_0);
        case 7: return (a.x_0 * a.z_1) - (a.z_0 * a.x_1);
        case 8: return (a.x_0 * a.y_1) - (a.x_1 * a.y_0);
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
inline T det(const PTMatrix3<T>& a) { return ((a.x_0 * minor(a, 0)) - (a.y_0 * minor(a, 1))) + (a.z_0 * minor(a, 2)); }

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
inline PTMatrix3<T> cofact(const PTMatrix3<T>& a) { return PTMatrix3<T>{ minor(a,0), -minor(a,1), minor(a,2), -minor(a,3), minor(a,4), -minor(a,5), minor(a,6), -minor(a,7), minor(a,8) }; }

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
inline PTMatrix3<T> adj(const PTMatrix3<T>& a) { return -cofact(a); }

typedef PTMatrix3<float> PTMatrix3f;