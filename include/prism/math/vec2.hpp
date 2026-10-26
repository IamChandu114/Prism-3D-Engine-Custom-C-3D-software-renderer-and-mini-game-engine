#pragma once

#include <cmath>
#include <prism/math/constants.hpp>

namespace prism::math {

struct Vec2 {
    float x{0.0f};
    float y{0.0f};

    constexpr Vec2() noexcept = default;
    constexpr Vec2(float x_, float y_) noexcept : x(x_), y(y_) {}
    explicit constexpr Vec2(float scalar) noexcept : x(scalar), y(scalar) {}

    [[nodiscard]] constexpr float operator[](int index) const noexcept {
        return (index == 0) ? x : y;
    }

    [[nodiscard]] float& operator[](int index) noexcept { return (index == 0) ? x : y; }

    constexpr Vec2& operator+=(const Vec2& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Vec2& operator-=(const Vec2& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr Vec2& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vec2& operator/=(float scalar) noexcept {
        float inv = 1.0f / scalar;
        x *= inv;
        y *= inv;
        return *this;
    }

    [[nodiscard]] constexpr Vec2 operator-() const noexcept { return Vec2(-x, -y); }
};

[[nodiscard]] constexpr Vec2 operator+(Vec2 a, Vec2 b) noexcept {
    return Vec2(a.x + b.x, a.y + b.y);
}

[[nodiscard]] constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept {
    return Vec2(a.x - b.x, a.y - b.y);
}

[[nodiscard]] constexpr Vec2 operator*(Vec2 a, float scalar) noexcept {
    return Vec2(a.x * scalar, a.y * scalar);
}

[[nodiscard]] constexpr Vec2 operator*(float scalar, Vec2 a) noexcept {
    return Vec2(a.x * scalar, a.y * scalar);
}

[[nodiscard]] constexpr Vec2 operator*(Vec2 a, Vec2 b) noexcept {
    return Vec2(a.x * b.x, a.y * b.y);
}

[[nodiscard]] constexpr Vec2 operator/(Vec2 a, float scalar) noexcept {
    float inv = 1.0f / scalar;
    return Vec2(a.x * inv, a.y * inv);
}

[[nodiscard]] constexpr float dot(Vec2 a, Vec2 b) noexcept {
    return a.x * b.x + a.y * b.y;
}

[[nodiscard]] constexpr float length_squared(Vec2 v) noexcept {
    return dot(v, v);
}

[[nodiscard]] inline float length(Vec2 v) noexcept {
    return std::sqrt(length_squared(v));
}

[[nodiscard]] inline Vec2 normalize(Vec2 v) noexcept {
    float len_sq = length_squared(v);
    if (len_sq > kEps * kEps) {
        float inv = 1.0f / std::sqrt(len_sq);
        return Vec2(v.x * inv, v.y * inv);
    }
    return Vec2(0.0f, 0.0f);
}

[[nodiscard]] constexpr Vec2 lerp(Vec2 a, Vec2 b, float t) noexcept {
    return Vec2(lerp(a.x, b.x, t), lerp(a.y, b.y, t));
}

[[nodiscard]] constexpr Vec2 reflect(Vec2 v, Vec2 n) noexcept {
    return v - 2.0f * dot(v, n) * n;
}

[[nodiscard]] constexpr Vec2 project(Vec2 v, Vec2 onto) noexcept {
    float len_sq = dot(onto, onto);
    if (len_sq > kEps * kEps) {
        return onto * (dot(v, onto) / len_sq);
    }
    return Vec2(0.0f, 0.0f);
}

} // namespace prism::math
