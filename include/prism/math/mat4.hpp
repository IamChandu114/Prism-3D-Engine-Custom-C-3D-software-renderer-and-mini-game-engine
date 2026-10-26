#pragma once

#include <prism/math/mat3.hpp>
#include <prism/math/vec3.hpp>
#include <prism/math/vec4.hpp>

namespace prism::math {

struct Mat4 {
    // 16 elements stored in column-major order:
    // col 0: [0, 1, 2, 3]
    // col 1: [4, 5, 6, 7]
    // col 2: [8, 9, 10, 11]
    // col 3: [12, 13, 14, 15]
    float elements[16]{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                       0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    constexpr Mat4() noexcept = default;

    // Constructed from row-major argument values for visual readability:
    constexpr Mat4(float m00, float m01, float m02, float m03, float m10, float m11, float m12,
                   float m13, float m20, float m21, float m22, float m23, float m30, float m31,
                   float m32, float m33) noexcept
        : elements{
              m00, m10, m20, m30, // Col 0
              m01, m11, m21, m31, // Col 1
              m02, m12, m22, m32, // Col 2
              m03, m13, m23, m33  // Col 3
          } {}

    [[nodiscard]] static constexpr Mat4 identity() noexcept {
        return Mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f);
    }

    [[nodiscard]] constexpr float operator()(int row, int col) const noexcept {
        return elements[col * 4 + row];
    }

    [[nodiscard]] float& operator()(int row, int col) noexcept { return elements[col * 4 + row]; }

    [[nodiscard]] const float* data() const noexcept { return elements; }

    [[nodiscard]] float* data() noexcept { return elements; }
};

[[nodiscard]] constexpr Mat4 operator*(const Mat4& a, const Mat4& b) noexcept {
    Mat4 res{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a(row, k) * b(k, col);
            }
            res(row, col) = sum;
        }
    }
    return res;
}

[[nodiscard]] constexpr Vec4 operator*(const Mat4& m, Vec4 v) noexcept {
    return Vec4(m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z + m(0, 3) * v.w,
                m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z + m(1, 3) * v.w,
                m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z + m(2, 3) * v.w,
                m(3, 0) * v.x + m(3, 1) * v.y + m(3, 2) * v.z + m(3, 3) * v.w);
}

[[nodiscard]] constexpr Mat4 transpose(const Mat4& m) noexcept {
    return Mat4(m(0, 0), m(1, 0), m(2, 0), m(3, 0), m(0, 1), m(1, 1), m(2, 1), m(3, 1), m(0, 2),
                m(1, 2), m(2, 2), m(3, 2), m(0, 3), m(1, 3), m(2, 3), m(3, 3));
}

[[nodiscard]] inline float determinant(const Mat4& m) noexcept {
    // 2x2 determinants for Laplace expansion
    float s0 = m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1);
    float s1 = m(0, 0) * m(1, 2) - m(1, 0) * m(0, 2);
    float s2 = m(0, 0) * m(1, 3) - m(1, 0) * m(0, 3);
    float s3 = m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2);
    float s4 = m(0, 1) * m(1, 3) - m(1, 1) * m(0, 3);
    float s5 = m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3);

    float c5 = m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3);
    float c4 = m(2, 1) * m(3, 3) - m(3, 1) * m(2, 3);
    float c3 = m(2, 1) * m(3, 2) - m(3, 1) * m(2, 2);
    float c2 = m(2, 0) * m(3, 3) - m(3, 0) * m(2, 3);
    float c1 = m(2, 0) * m(3, 2) - m(3, 0) * m(2, 2);
    float c0 = m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1);

    return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
}

[[nodiscard]] inline Mat4 inverse(const Mat4& m) noexcept {
    float s0 = m(0, 0) * m(1, 1) - m(1, 0) * m(0, 1);
    float s1 = m(0, 0) * m(1, 2) - m(1, 0) * m(0, 2);
    float s2 = m(0, 0) * m(1, 3) - m(1, 0) * m(0, 3);
    float s3 = m(0, 1) * m(1, 2) - m(1, 1) * m(0, 2);
    float s4 = m(0, 1) * m(1, 3) - m(1, 1) * m(0, 3);
    float s5 = m(0, 2) * m(1, 3) - m(1, 2) * m(0, 3);

    float c5 = m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3);
    float c4 = m(2, 1) * m(3, 3) - m(3, 1) * m(2, 3);
    float c3 = m(2, 1) * m(3, 2) - m(3, 1) * m(2, 2);
    float c2 = m(2, 0) * m(3, 3) - m(3, 0) * m(2, 3);
    float c1 = m(2, 0) * m(3, 2) - m(3, 0) * m(2, 2);
    float c0 = m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1);

    float det = s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    if (std::abs(det) < kEps) {
        return Mat4::identity();
    }

    float inv_det = 1.0f / det;
    Mat4 inv{};

    inv(0, 0) = (m(1, 1) * c5 - m(1, 2) * c4 + m(1, 3) * c3) * inv_det;
    inv(0, 1) = (-m(0, 1) * c5 + m(0, 2) * c4 - m(0, 3) * c3) * inv_det;
    inv(0, 2) = (m(3, 1) * s5 - m(3, 2) * s4 + m(3, 3) * s3) * inv_det;
    inv(0, 3) = (-m(2, 1) * s5 + m(2, 2) * s4 - m(2, 3) * s3) * inv_det;

    inv(1, 0) = (-m(1, 0) * c5 + m(1, 2) * c2 - m(1, 3) * c1) * inv_det;
    inv(1, 1) = (m(0, 0) * c5 - m(0, 2) * c2 + m(0, 3) * c1) * inv_det;
    inv(1, 2) = (-m(3, 0) * s5 + m(3, 2) * s2 - m(3, 3) * s1) * inv_det;
    inv(1, 3) = (m(2, 0) * s5 - m(2, 2) * s2 + m(2, 3) * s1) * inv_det;

    inv(2, 0) = (m(1, 0) * c4 - m(1, 1) * c2 + m(1, 3) * c0) * inv_det;
    inv(2, 1) = (-m(0, 0) * c4 + m(0, 1) * c2 - m(0, 3) * c0) * inv_det;
    inv(2, 2) = (m(3, 0) * s4 - m(3, 1) * s2 + m(3, 3) * s0) * inv_det;
    inv(2, 3) = (-m(2, 0) * s4 + m(2, 1) * s2 - m(2, 3) * s0) * inv_det;

    inv(3, 0) = (-m(1, 0) * c3 + m(1, 1) * c1 - m(1, 2) * c0) * inv_det;
    inv(3, 1) = (m(0, 0) * c3 - m(0, 1) * c1 + m(0, 2) * c0) * inv_det;
    inv(3, 2) = (-m(3, 0) * s3 + m(3, 1) * s1 - m(3, 2) * s0) * inv_det;
    inv(3, 3) = (m(2, 0) * s3 - m(2, 1) * s1 + m(2, 2) * s0) * inv_det;

    return inv;
}

// Fast path for Affine transformations [R | T; 0 0 0 1]
[[nodiscard]] inline Mat4 inverse_affine(const Mat4& m) noexcept {
    Mat3 r(m(0, 0), m(0, 1), m(0, 2), m(1, 0), m(1, 1), m(1, 2), m(2, 0), m(2, 1), m(2, 2));
    Mat3 inv_r = inverse(r);
    Vec3 t(m(0, 3), m(1, 3), m(2, 3));
    Vec3 inv_t = -(inv_r * t);

    return Mat4(inv_r(0, 0), inv_r(0, 1), inv_r(0, 2), inv_t.x, inv_r(1, 0), inv_r(1, 1),
                inv_r(1, 2), inv_t.y, inv_r(2, 0), inv_r(2, 1), inv_r(2, 2), inv_t.z, 0.0f, 0.0f,
                0.0f, 1.0f);
}

} // namespace prism::math
