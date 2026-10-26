#pragma once

#include <cmath>
#include <prism/math/constants.hpp>
#include <prism/math/mat4.hpp>
#include <prism/math/vec3.hpp>

namespace prism::math {

struct Quat {
    float w{1.0f};
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    constexpr Quat() noexcept = default;
    constexpr Quat(float w_, float x_, float y_, float z_) noexcept : w(w_), x(x_), y(y_), z(z_) {}

    [[nodiscard]] static constexpr Quat identity() noexcept { return Quat(1.0f, 0.0f, 0.0f, 0.0f); }

    [[nodiscard]] static inline Quat from_axis_angle(Vec3 axis, float angle_rad) noexcept {
        Vec3 u = normalize(axis);
        float half = angle_rad * 0.5f;
        float s = std::sin(half);
        return Quat(std::cos(half), u.x * s, u.y * s, u.z * s);
    }

    [[nodiscard]] constexpr Vec3 v() const noexcept { return Vec3(x, y, z); }

    [[nodiscard]] inline Vec3 rotate(Vec3 pt) const noexcept {
        // v' = v + 2 * q_v x (q_v x v + w * v)
        Vec3 qv(x, y, z);
        Vec3 t = 2.0f * cross(qv, pt);
        return pt + w * t + cross(qv, t);
    }

    [[nodiscard]] inline Mat4 to_mat4() const noexcept {
        float xx = x * x;
        float yy = y * y;
        float zz = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        return Mat4(1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz), 2.0f * (xz + wy), 0.0f,
                    2.0f * (xy + wz), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx), 0.0f,
                    2.0f * (xz - wy), 2.0f * (yz + wx), 1.0f - 2.0f * (xx + yy), 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f);
    }
};

[[nodiscard]] constexpr Quat operator+(Quat a, Quat b) noexcept {
    return Quat(a.w + b.w, a.x + b.x, a.y + b.y, a.z + b.z);
}

[[nodiscard]] constexpr Quat operator-(Quat a, Quat b) noexcept {
    return Quat(a.w - b.w, a.x - b.x, a.y - b.y, a.z - b.z);
}

[[nodiscard]] constexpr Quat operator*(Quat q, float s) noexcept {
    return Quat(q.w * s, q.x * s, q.y * s, q.z * s);
}

[[nodiscard]] constexpr Quat operator*(float s, Quat q) noexcept {
    return Quat(q.w * s, q.x * s, q.y * s, q.z * s);
}

[[nodiscard]] constexpr Quat operator*(Quat a, Quat b) noexcept {
    return Quat(a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
                a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
                a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
                a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w);
}

[[nodiscard]] constexpr float dot(Quat a, Quat b) noexcept {
    return a.w * b.w + a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] constexpr float length_squared(Quat q) noexcept {
    return dot(q, q);
}

[[nodiscard]] inline float length(Quat q) noexcept {
    return std::sqrt(length_squared(q));
}

[[nodiscard]] inline Quat normalize(Quat q) noexcept {
    float len_sq = length_squared(q);
    if (len_sq > kEps * kEps) {
        float inv = 1.0f / std::sqrt(len_sq);
        return q * inv;
    }
    return Quat::identity();
}

[[nodiscard]] constexpr Quat conjugate(Quat q) noexcept {
    return Quat(q.w, -q.x, -q.y, -q.z);
}

[[nodiscard]] inline Quat inverse(Quat q) noexcept {
    float len_sq = length_squared(q);
    if (len_sq > kEps * kEps) {
        return conjugate(q) * (1.0f / len_sq);
    }
    return Quat::identity();
}

[[nodiscard]] inline Quat nlerp(Quat a, Quat b, float t) noexcept {
    float cos_theta = dot(a, b);
    Quat b_adj = (cos_theta < 0.0f) ? (b * -1.0f) : b;
    return normalize(a * (1.0f - t) + b_adj * t);
}

[[nodiscard]] inline Quat slerp(Quat a, Quat b, float t) noexcept {
    float cos_theta = dot(a, b);
    Quat b_adj = b;

    // Shortest path interpolation
    if (cos_theta < 0.0f) {
        cos_theta = -cos_theta;
        b_adj = b * -1.0f;
    }

    // Near parallel: fallback to linear interpolation to prevent 0 / 0
    if (cos_theta > 0.9995f) {
        return normalize(a * (1.0f - t) + b_adj * t);
    }

    cos_theta = clamp(cos_theta, -1.0f, 1.0f);
    float theta = std::acos(cos_theta);
    float sin_theta = std::sin(theta);

    float scale_a = std::sin((1.0f - t) * theta) / sin_theta;
    float scale_b = std::sin(t * theta) / sin_theta;

    return a * scale_a + b_adj * scale_b;
}

} // namespace prism::math
