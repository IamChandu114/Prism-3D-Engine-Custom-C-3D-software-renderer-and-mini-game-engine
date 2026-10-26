#pragma once

#include <algorithm>
#include <cmath>

namespace prism::math {

inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kTwoPi = 6.28318530717958647692f;
inline constexpr float kHalfPi = 1.57079632679489661923f;
inline constexpr float kInvPi = 0.31830988618379067154f;
inline constexpr float kEps = 1e-6f;

[[nodiscard]] constexpr float deg_to_rad(float deg) noexcept {
    return deg * (kPi / 180.0f);
}

[[nodiscard]] constexpr float rad_to_deg(float rad) noexcept {
    return rad * (180.0f / kPi);
}

[[nodiscard]] inline bool approx_equal(float a, float b, float epsilon = kEps) noexcept {
    return std::abs(a - b) <= epsilon;
}

[[nodiscard]] constexpr float clamp(float v, float lo, float hi) noexcept {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

[[nodiscard]] constexpr float lerp(float a, float b, float t) noexcept {
    return a + t * (b - a);
}

} // namespace prism::math
