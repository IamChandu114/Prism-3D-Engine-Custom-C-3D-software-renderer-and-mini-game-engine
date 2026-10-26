#pragma once

#include <algorithm>
#include <cmath>
#include <prism/math/constants.hpp>
#include <prism/math/mat4.hpp>
#include <prism/math/vec2.hpp>
#include <prism/math/vec3.hpp>

namespace prism::math {

[[nodiscard]] inline Vec3 barycentric(Vec2 p, Vec2 a, Vec2 b, Vec2 c) noexcept {
    float area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (std::abs(area) < kEps) {
        return Vec3(0.0f, 0.0f, 0.0f);
    }
    float inv_area = 1.0f / area;

    float alpha = ((b.x - p.x) * (c.y - p.y) - (b.y - p.y) * (c.x - p.x)) * inv_area;
    float beta = ((c.x - p.x) * (a.y - p.y) - (c.y - p.y) * (a.x - p.x)) * inv_area;
    float gamma = 1.0f - alpha - beta;

    return Vec3(alpha, beta, gamma);
}

struct Plane {
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float distance{0.0f}; // d in n . x + d = 0

    constexpr Plane() noexcept = default;
    constexpr Plane(Vec3 n, float d) noexcept : normal(n), distance(d) {}

    [[nodiscard]] static inline Plane from_points(Vec3 p0, Vec3 p1, Vec3 p2) noexcept {
        Vec3 n = normalize(cross(p1 - p0, p2 - p0));
        float d = -dot(n, p0);
        return Plane(n, d);
    }

    [[nodiscard]] static inline Plane from_point_normal(Vec3 pt, Vec3 n) noexcept {
        Vec3 norm = normalize(n);
        return Plane(norm, -dot(norm, pt));
    }

    [[nodiscard]] constexpr float signed_distance(Vec3 pt) const noexcept {
        return dot(normal, pt) + distance;
    }

    inline void normalize_plane() noexcept {
        float len = length(normal);
        if (len > kEps) {
            float inv = 1.0f / len;
            normal = normal * inv;
            distance *= inv;
        }
    }
};

struct AABB {
    Vec3 min{1e30f, 1e30f, 1e30f};
    Vec3 max{-1e30f, -1e30f, -1e30f};

    constexpr AABB() noexcept = default;
    constexpr AABB(Vec3 min_, Vec3 max_) noexcept : min(min_), max(max_) {}

    [[nodiscard]] constexpr Vec3 center() const noexcept { return (min + max) * 0.5f; }

    [[nodiscard]] constexpr Vec3 extents() const noexcept { return (max - min) * 0.5f; }

    [[nodiscard]] constexpr bool contains(Vec3 pt) const noexcept {
        return (pt.x >= min.x && pt.x <= max.x) && (pt.y >= min.y && pt.y <= max.y) &&
               (pt.z >= min.z && pt.z <= max.z);
    }

    constexpr void expand(Vec3 pt) noexcept {
        min.x = std::min(min.x, pt.x);
        min.y = std::min(min.y, pt.y);
        min.z = std::min(min.z, pt.z);
        max.x = std::max(max.x, pt.x);
        max.y = std::max(max.y, pt.y);
        max.z = std::max(max.z, pt.z);
    }

    constexpr void expand(const AABB& other) noexcept {
        min.x = std::min(min.x, other.min.x);
        min.y = std::min(min.y, other.min.y);
        min.z = std::min(min.z, other.min.z);
        max.x = std::max(max.x, other.max.x);
        max.y = std::max(max.y, other.max.y);
        max.z = std::max(max.z, other.max.z);
    }
};

struct Sphere {
    Vec3 center{0.0f, 0.0f, 0.0f};
    float radius{1.0f};

    constexpr Sphere() noexcept = default;
    constexpr Sphere(Vec3 c, float r) noexcept : center(c), radius(r) {}

    [[nodiscard]] inline bool contains(Vec3 pt) const noexcept {
        return length_squared(pt - center) <= radius * radius;
    }
};

struct Ray {
    Vec3 origin{0.0f, 0.0f, 0.0f};
    Vec3 direction{0.0f, 0.0f, 1.0f};

    constexpr Ray() noexcept = default;
    Ray(Vec3 o, Vec3 d) noexcept : origin(o), direction(normalize(d)) {}

    [[nodiscard]] constexpr Vec3 point_at(float t) const noexcept { return origin + direction * t; }
};

struct Frustum {
    // 6 clipping planes: Left, Right, Bottom, Top, Near, Far
    enum PlaneIndex { Left = 0, Right, Bottom, Top, Near, Far, Count };
    Plane planes[Count];

    [[nodiscard]] static inline Frustum from_view_projection(const Mat4& m) noexcept {
        Frustum f;

        // Row vectors of m
        Vec4 r0(m(0, 0), m(0, 1), m(0, 2), m(0, 3));
        Vec4 r1(m(1, 0), m(1, 1), m(1, 2), m(1, 3));
        Vec4 r2(m(2, 0), m(2, 1), m(2, 2), m(2, 3));
        Vec4 r3(m(3, 0), m(3, 1), m(3, 2), m(3, 3));

        // Gribb-Hartmann extraction for standard NDC [-1, 1]
        Vec4 left = r3 + r0;
        Vec4 right = r3 - r0;
        Vec4 bottom = r3 + r1;
        Vec4 top = r3 - r1;
        Vec4 near_p = r3 + r2;
        Vec4 far_p = r3 - r2;

        f.planes[Left] = Plane(left.xyz(), left.w);
        f.planes[Right] = Plane(right.xyz(), right.w);
        f.planes[Bottom] = Plane(bottom.xyz(), bottom.w);
        f.planes[Top] = Plane(top.xyz(), top.w);
        f.planes[Near] = Plane(near_p.xyz(), near_p.w);
        f.planes[Far] = Plane(far_p.xyz(), far_p.w);

        for (int i = 0; i < Count; ++i) {
            f.planes[i].normalize_plane();
        }

        return f;
    }

    [[nodiscard]] inline bool contains_point(Vec3 pt) const noexcept {
        for (int i = 0; i < Count; ++i) {
            if (planes[i].signed_distance(pt) < 0.0f) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] inline bool intersects_sphere(const Sphere& s) const noexcept {
        for (int i = 0; i < Count; ++i) {
            if (planes[i].signed_distance(s.center) < -s.radius) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] inline bool intersects_aabb(const AABB& b) const noexcept {
        for (int i = 0; i < Count; ++i) {
            // Find positive vertex along plane normal
            Vec3 p((planes[i].normal.x >= 0.0f) ? b.max.x : b.min.x,
                   (planes[i].normal.y >= 0.0f) ? b.max.y : b.min.y,
                   (planes[i].normal.z >= 0.0f) ? b.max.z : b.min.z);
            if (planes[i].signed_distance(p) < 0.0f) {
                return false;
            }
        }
        return true;
    }
};

} // namespace prism::math
