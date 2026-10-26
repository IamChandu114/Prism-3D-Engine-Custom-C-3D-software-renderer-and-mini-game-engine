#pragma once

#include <cmath>
#include <prism/math/constants.hpp>

namespace prism::math {

struct Vec3 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};

    constexpr Vec3() noexcept = default;
    constexpr Vec3(float x_, float y_, float z_) noexcept : x(x_), y(y_), z(z_) {}
    explicit constexpr Vec3(float scalar) noexcept : x(scalar), y(scalar), z(scalar) {}

    [[nodiscard]] constexpr float operator[](int index) const noexcept {
        return (index == 0) ? x : ((index == 1) ? y : z);
    }

    [[nodiscard]] float& operator[](int index) noexcept {
        return (index == 0) ? x : ((index == 1) ? y : z);
    }

    constexpr Vec3& operator+=(const Vec3& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vec3& operator-=(const Vec3& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr Vec3& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vec3& operator/=(float scalar) noexcept {
        float inv = 1.0f / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
        return *this;
    }

    [[nodiscard]] constexpr Vec3 operator-() const noexcept { return Vec3(-x, -y, -z); }
};

[[nodiscard]] constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept {
    return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

[[nodiscard]] constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept {
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

[[nodiscard]] constexpr Vec3 operator*(Vec3 a, float scalar) noexcept {
    return Vec3(a.x * scalar, a.y * scalar, a.z * scalar);
}

[[nodiscard]] constexpr Vec3 operator*(float scalar, Vec3 a) noexcept {
    return Vec3(a.x * scalar, a.y * scalar, a.z * scalar);
}

[[nodiscard]] constexpr Vec3 operator*(Vec3 a, Vec3 b) noexcept {
    return Vec3(a.x * b.x, a.y * b.y, a.z * b.z);
}

[[nodiscard]] constexpr Vec3 operator/(Vec3 a, float scalar) noexcept {
    float inv = 1.0f / scalar;
    return Vec3(a.x * inv, a.y * inv, a.z * inv);
}

[[nodiscard]] constexpr Vec3 operator/(Vec3 a, Vec3 b) noexcept {
    return Vec3(a.x / b.x, a.y / b.y, a.z / b.z);
}

[[nodiscard]] constexpr float dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] constexpr Vec3 cross(Vec3 a, Vec3 b) noexcept {
    return Vec3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

[[nodiscard]] constexpr float length_squared(Vec3 v) noexcept {
    return dot(v, v);
}

[[nodiscard]] inline float length(Vec3 v) noexcept {
    return std::sqrt(length_squared(v));
}

[[nodiscard]] inline Vec3 normalize(Vec3 v) noexcept {
    float len_sq = length_squared(v);
    if (len_sq > kEps * kEps) {
        float inv = 1.0f / std::sqrt(len_sq);
        return Vec3(v.x * inv, v.y * inv, v.z * inv);
    }
    return Vec3(0.0f, 0.0f, 0.0f);
}

[[nodiscard]] constexpr Vec3 lerp(Vec3 a, Vec3 b, float t) noexcept {
    return Vec3(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t));
}

[[nodiscard]] constexpr Vec3 reflect(Vec3 v, Vec3 n) noexcept {
    return v - 2.0f * dot(v, n) * n;
}

[[nodiscard]] constexpr Vec3 project(Vec3 v, Vec3 onto) noexcept {
    float len_sq = dot(onto, onto);
    if (len_sq > kEps * kEps) {
        return onto * (dot(v, onto) / len_sq);
    }
    return Vec3(0.0f, 0.0f, 0.0f);
}

} // namespace prism::math
