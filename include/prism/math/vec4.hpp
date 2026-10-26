#pragma once

#include <prism/math/vec3.hpp>

namespace prism::math {

struct Vec4 {
    float x{0.0f};
    float y{0.0f};
    float z{0.0f};
    float w{0.0f};

    constexpr Vec4() noexcept = default;
    constexpr Vec4(float x_, float y_, float z_, float w_) noexcept : x(x_), y(y_), z(z_), w(w_) {}
    constexpr Vec4(Vec3 v, float w_) noexcept : x(v.x), y(v.y), z(v.z), w(w_) {}
    explicit constexpr Vec4(float scalar) noexcept : x(scalar), y(scalar), z(scalar), w(scalar) {}

    [[nodiscard]] constexpr Vec3 xyz() const noexcept { return Vec3(x, y, z); }

    [[nodiscard]] constexpr float operator[](int index) const noexcept {
        return (index == 0) ? x : ((index == 1) ? y : ((index == 2) ? z : w));
    }

    [[nodiscard]] float& operator[](int index) noexcept {
        return (index == 0) ? x : ((index == 1) ? y : ((index == 2) ? z : w));
    }

    constexpr Vec4& operator+=(const Vec4& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    constexpr Vec4& operator-=(const Vec4& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    constexpr Vec4& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;
        return *this;
    }

    constexpr Vec4& operator/=(float scalar) noexcept {
        float inv = 1.0f / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
        w *= inv;
        return *this;
    }

    [[nodiscard]] constexpr Vec4 operator-() const noexcept { return Vec4(-x, -y, -z, -w); }
};

[[nodiscard]] constexpr Vec4 operator+(Vec4 a, Vec4 b) noexcept {
    return Vec4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

[[nodiscard]] constexpr Vec4 operator-(Vec4 a, Vec4 b) noexcept {
    return Vec4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

[[nodiscard]] constexpr Vec4 operator*(Vec4 a, float scalar) noexcept {
    return Vec4(a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar);
}

[[nodiscard]] constexpr Vec4 operator*(float scalar, Vec4 a) noexcept {
    return Vec4(a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar);
}

[[nodiscard]] constexpr Vec4 operator*(Vec4 a, Vec4 b) noexcept {
    return Vec4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

[[nodiscard]] constexpr Vec4 operator/(Vec4 a, float scalar) noexcept {
    float inv = 1.0f / scalar;
    return Vec4(a.x * inv, a.y * inv, a.z * inv, a.w * inv);
}

[[nodiscard]] constexpr float dot(Vec4 a, Vec4 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

[[nodiscard]] constexpr float length_squared(Vec4 v) noexcept {
    return dot(v, v);
}

[[nodiscard]] inline float length(Vec4 v) noexcept {
    return std::sqrt(length_squared(v));
}

[[nodiscard]] inline Vec4 normalize(Vec4 v) noexcept {
    float len_sq = length_squared(v);
    if (len_sq > kEps * kEps) {
        float inv = 1.0f / std::sqrt(len_sq);
        return Vec4(v.x * inv, v.y * inv, v.z * inv, v.w * inv);
    }
    return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
}

[[nodiscard]] constexpr Vec4 lerp(Vec4 a, Vec4 b, float t) noexcept {
    return Vec4(lerp(a.x, b.x, t), lerp(a.y, b.y, t), lerp(a.z, b.z, t), lerp(a.w, b.w, t));
}

} // namespace prism::math
