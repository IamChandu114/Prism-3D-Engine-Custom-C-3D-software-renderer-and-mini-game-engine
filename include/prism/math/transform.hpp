#pragma once

#include <cmath>
#include <prism/math/constants.hpp>
#include <prism/math/mat4.hpp>
#include <prism/math/vec3.hpp>

namespace prism::math {

[[nodiscard]] inline Mat4 translate(Vec3 t) noexcept {
    return Mat4(1.0f, 0.0f, 0.0f, t.x, 0.0f, 1.0f, 0.0f, t.y, 0.0f, 0.0f, 1.0f, t.z, 0.0f, 0.0f,
                0.0f, 1.0f);
}

[[nodiscard]] inline Mat4 scale(Vec3 s) noexcept {
    return Mat4(s.x, 0.0f, 0.0f, 0.0f, 0.0f, s.y, 0.0f, 0.0f, 0.0f, 0.0f, s.z, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f);
}

[[nodiscard]] inline Mat4 scale(float uniform_s) noexcept {
    return scale(Vec3(uniform_s, uniform_s, uniform_s));
}

[[nodiscard]] inline Mat4 rotate_x(float rad) noexcept {
    float c = std::cos(rad);
    float s = std::sin(rad);
    return Mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, c, -s, 0.0f, 0.0f, s, c, 0.0f, 0.0f, 0.0f, 0.0f,
                1.0f);
}

[[nodiscard]] inline Mat4 rotate_y(float rad) noexcept {
    float c = std::cos(rad);
    float s = std::sin(rad);
    return Mat4(c, 0.0f, s, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -s, 0.0f, c, 0.0f, 0.0f, 0.0f, 0.0f,
                1.0f);
}

[[nodiscard]] inline Mat4 rotate_z(float rad) noexcept {
    float c = std::cos(rad);
    float s = std::sin(rad);
    return Mat4(c, -s, 0.0f, 0.0f, s, c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                1.0f);
}

[[nodiscard]] inline Mat4 rotate_axis_angle(Vec3 axis, float rad) noexcept {
    Vec3 a = normalize(axis);
    float c = std::cos(rad);
    float s = std::sin(rad);
    float t = 1.0f - c;

    return Mat4(t * a.x * a.x + c, t * a.x * a.y - s * a.z, t * a.x * a.z + s * a.y, 0.0f,
                t * a.x * a.y + s * a.z, t * a.y * a.y + c, t * a.y * a.z - s * a.x, 0.0f,
                t * a.x * a.z - s * a.y, t * a.y * a.z + s * a.x, t * a.z * a.z + c, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f);
}

[[nodiscard]] inline Mat4 look_at(Vec3 eye, Vec3 target, Vec3 up) noexcept {
    // Forward direction from eye to target
    Vec3 f = normalize(target - eye);
    // Right vector
    Vec3 r = normalize(cross(f, up));
    // Recomputed true orthogonal up vector
    Vec3 u = cross(r, f);

    // In right-handed coordinates, camera looks down -Z:
    // row 0: r
    // row 1: u
    // row 2: -f
    return Mat4(r.x, r.y, r.z, -dot(r, eye), u.x, u.y, u.z, -dot(u, eye), -f.x, -f.y, -f.z,
                dot(f, eye), 0.0f, 0.0f, 0.0f, 1.0f);
}

[[nodiscard]] inline Mat4 perspective(float fov_y_rad, float aspect, float near_z,
                                      float far_z) noexcept {
    float tan_half_fov = std::tan(fov_y_rad * 0.5f);
    float f_minus_n = far_z - near_z;

    float a = 1.0f / (aspect * tan_half_fov);
    float b = 1.0f / tan_half_fov;
    float c = -(far_z + near_z) / f_minus_n;
    float d = -(2.0f * far_z * near_z) / f_minus_n;

    return Mat4(a, 0.0f, 0.0f, 0.0f, 0.0f, b, 0.0f, 0.0f, 0.0f, 0.0f, c, d, 0.0f, 0.0f, -1.0f,
                0.0f);
}

[[nodiscard]] inline Mat4 orthographic(float left, float right, float bottom, float top,
                                       float near_z, float far_z) noexcept {
    float r_minus_l = right - left;
    float t_minus_b = top - bottom;
    float f_minus_n = far_z - near_z;

    return Mat4(2.0f / r_minus_l, 0.0f, 0.0f, -(right + left) / r_minus_l, 0.0f, 2.0f / t_minus_b,
                0.0f, -(top + bottom) / t_minus_b, 0.0f, 0.0f, -2.0f / f_minus_n,
                -(far_z + near_z) / f_minus_n, 0.0f, 0.0f, 0.0f, 1.0f);
}

} // namespace prism::math
