#pragma once

#include <prism/math/vec3.hpp>

namespace prism::math {

struct Mat3 {
    // 9 elements in column-major order:
    // col 0: [0, 1, 2]
    // col 1: [3, 4, 5]
    // col 2: [6, 7, 8]
    float elements[9]{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    constexpr Mat3() noexcept = default;

    // Constructed using row-major argument list for intuitive visual reading in code:
    constexpr Mat3(float m00, float m01, float m02, float m10, float m11, float m12, float m20,
                   float m21, float m22) noexcept
        : elements{
              m00, m10, m20, // Column 0
              m01, m11, m21, // Column 1
              m02, m12, m22  // Column 2
          } {}

    [[nodiscard]] static constexpr Mat3 identity() noexcept {
        return Mat3(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    }

    [[nodiscard]] constexpr float operator()(int row, int col) const noexcept {
        return elements[col * 3 + row];
    }

    [[nodiscard]] float& operator()(int row, int col) noexcept { return elements[col * 3 + row]; }

    [[nodiscard]] const float* data() const noexcept { return elements; }

    [[nodiscard]] float* data() noexcept { return elements; }
};

[[nodiscard]] constexpr Mat3 operator*(const Mat3& a, const Mat3& b) noexcept {
    Mat3 res{};
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 3; ++k) {
                sum += a(row, k) * b(k, col);
            }
            res(row, col) = sum;
        }
    }
    return res;
}

[[nodiscard]] constexpr Vec3 operator*(const Mat3& m, Vec3 v) noexcept {
    return Vec3(m(0, 0) * v.x + m(0, 1) * v.y + m(0, 2) * v.z,
                m(1, 0) * v.x + m(1, 1) * v.y + m(1, 2) * v.z,
                m(2, 0) * v.x + m(2, 1) * v.y + m(2, 2) * v.z);
}

[[nodiscard]] constexpr Mat3 transpose(const Mat3& m) noexcept {
    return Mat3(m(0, 0), m(1, 0), m(2, 0), m(0, 1), m(1, 1), m(2, 1), m(0, 2), m(1, 2), m(2, 2));
}

[[nodiscard]] constexpr float determinant(const Mat3& m) noexcept {
    return m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) -
           m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) +
           m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));
}

[[nodiscard]] inline Mat3 inverse(const Mat3& m) noexcept {
    float det = determinant(m);
    if (std::abs(det) < kEps) {
        return Mat3::identity();
    }

    float inv_det = 1.0f / det;
    Mat3 inv{};
    inv(0, 0) = (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) * inv_det;
    inv(0, 1) = -(m(0, 1) * m(2, 2) - m(0, 2) * m(2, 1)) * inv_det;
    inv(0, 2) = (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * inv_det;

    inv(1, 0) = -(m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) * inv_det;
    inv(1, 1) = (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0)) * inv_det;
    inv(1, 2) = -(m(0, 0) * m(1, 2) - m(0, 2) * m(1, 0)) * inv_det;

    inv(2, 0) = (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0)) * inv_det;
    inv(2, 1) = -(m(0, 0) * m(2, 1) - m(0, 1) * m(2, 0)) * inv_det;
    inv(2, 2) = (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) * inv_det;

    return inv;
}

} // namespace prism::math
