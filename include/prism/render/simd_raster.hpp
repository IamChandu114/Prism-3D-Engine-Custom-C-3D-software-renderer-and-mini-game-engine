#pragma once

#include <cstdint>
#include <prism/math/vec3.hpp>
#include <prism/math/vec4.hpp>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <immintrin.h>
#define PRISM_HAS_SIMD 1
#else
#define PRISM_HAS_SIMD 0
#endif

namespace prism::render {

// Evaluates 4 edge tests (px[0..3], py) against edge (ax, ay) -> (bx, by)
// Edge function: E(p) = (bx - ax)*(py - ay) - (by - ay)*(px - ax)
struct SimdEdge4 {
    float e[4];
};

[[nodiscard]] inline SimdEdge4 edge_function_4x_scalar(float ax, float ay, float bx, float by,
                                                       float px0, float px1, float px2, float px3,
                                                       float py) noexcept {
    float dx = bx - ax;
    float dy = by - ay;
    float py_ay = py - ay;
    SimdEdge4 res;
    res.e[0] = dx * py_ay - dy * (px0 - ax);
    res.e[1] = dx * py_ay - dy * (px1 - ax);
    res.e[2] = dx * py_ay - dy * (px2 - ax);
    res.e[3] = dx * py_ay - dy * (px3 - ax);
    return res;
}

[[nodiscard]] inline SimdEdge4 edge_function_4x_simd(float ax, float ay, float bx, float by,
                                                     float px0, float px1, float px2, float px3,
                                                     float py) noexcept {
#if PRISM_HAS_SIMD
    // Vectorized 4-wide edge function using SSE/AVX
    __m128 v_dx = _mm_set1_ps(bx - ax);
    __m128 v_dy = _mm_set1_ps(by - ay);
    __m128 v_py_ay = _mm_set1_ps(py - ay);
    __m128 v_ax = _mm_set1_ps(ax);
    __m128 v_px = _mm_set_ps(px3, px2, px1, px0); // [px0, px1, px2, px3]

    __m128 term1 = _mm_mul_ps(v_dx, v_py_ay);
    __m128 term2 = _mm_mul_ps(v_dy, _mm_sub_ps(v_px, v_ax));
    __m128 v_res = _mm_sub_ps(term1, term2);

    SimdEdge4 res;
    _mm_storeu_ps(res.e, v_res);
    return res;
#else
    return edge_function_4x_scalar(ax, ay, bx, by, px0, px1, px2, px3, py);
#endif
}

} // namespace prism::render
