#pragma once

#include <prism/math/mat4.hpp>
#include <prism/math/vec4.hpp>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define PRISM_SIMD_AVAILABLE 1
#else
#define PRISM_SIMD_AVAILABLE 0
#endif

namespace prism::math {

// Scalar matrix-vector multiplication
[[nodiscard]] inline Vec4 mat4_mul_vec4_scalar(const Mat4& m, const Vec4& v) noexcept {
    return Vec4(m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z + m(0, 3) * v.w,
                m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z + m(1, 3) * v.w,
                m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z + m(2, 3) * v.w,
                m(3, 0) * v.x + m(3, 1) * v.y + m(3, 2) * v.z + m(3, 3) * v.w);
}

// SIMD matrix-vector multiplication using column linear combination:
// v' = v.x * col0 + v.y * col1 + v.z * col2 + v.w * col3
[[nodiscard]] inline Vec4 mat4_mul_vec4_simd(const Mat4& m, const Vec4& v) noexcept {
#if PRISM_SIMD_AVAILABLE
    const float* d =
        m.data(); // column-major: d[0..3]=col0, d[4..7]=col1, d[8..11]=col2, d[12..15]=col3
    __m128 col0 = _mm_loadu_ps(d + 0);
    __m128 col1 = _mm_loadu_ps(d + 4);
    __m128 col2 = _mm_loadu_ps(d + 8);
    __m128 col3 = _mm_loadu_ps(d + 12);

    __m128 vx = _mm_set1_ps(v.x);
    __m128 vy = _mm_set1_ps(v.y);
    __m128 vz = _mm_set1_ps(v.z);
    __m128 vw = _mm_set1_ps(v.w);

    __m128 res = _mm_add_ps(_mm_add_ps(_mm_mul_ps(col0, vx), _mm_mul_ps(col1, vy)),
                            _mm_add_ps(_mm_mul_ps(col2, vz), _mm_mul_ps(col3, vw)));

    alignas(16) float out[4];
    _mm_storeu_ps(out, res);
    return Vec4(out[0], out[1], out[2], out[3]);
#else
    return mat4_mul_vec4_scalar(m, v);
#endif
}

} // namespace prism::math
