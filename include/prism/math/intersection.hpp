#pragma once

#include <algorithm>
#include <cmath>
#include <prism/math/geometry.hpp>
#include <prism/math/vec3.hpp>

namespace prism::math {

struct RayTriangleHit {
    float t{0.0f};
    float u{0.0f};
    float v{0.0f};
    Vec3 point{0.0f, 0.0f, 0.0f};
};

[[nodiscard]] inline bool intersect_ray_triangle(const Ray& ray, Vec3 v0, Vec3 v1, Vec3 v2,
                                                 RayTriangleHit& out_hit) noexcept {
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    Vec3 pvec = cross(ray.direction, edge2);
    float det = dot(edge1, pvec);

    if (std::abs(det) < kEps) {
        return false; // Ray is parallel to triangle plane
    }

    float inv_det = 1.0f / det;
    Vec3 tvec = ray.origin - v0;

    float u = dot(tvec, pvec) * inv_det;
    if (u < 0.0f || u > 1.0f) {
        return false;
    }

    Vec3 qvec = cross(tvec, edge1);
    float v = dot(ray.direction, qvec) * inv_det;
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }

    float t = dot(edge2, qvec) * inv_det;
    if (t < kEps) {
        return false; // Intersection behind ray origin
    }

    out_hit.t = t;
    out_hit.u = u;
    out_hit.v = v;
    out_hit.point = ray.point_at(t);
    return true;
}

[[nodiscard]] inline bool intersect_ray_aabb(const Ray& ray, const AABB& box, float& out_t_min,
                                             float& out_t_max) noexcept {
    float t_min = -1e30f;
    float t_max = 1e30f;

    for (int i = 0; i < 3; ++i) {
        float inv_d = (std::abs(ray.direction[i]) > kEps) ? (1.0f / ray.direction[i]) : 0.0f;
        if (std::abs(ray.direction[i]) > kEps) {
            float t0 = (box.min[i] - ray.origin[i]) * inv_d;
            float t1 = (box.max[i] - ray.origin[i]) * inv_d;
            if (t0 > t1) {
                std::swap(t0, t1);
            }
            t_min = std::max(t_min, t0);
            t_max = std::min(t_max, t1);
            if (t_min > t_max) {
                return false;
            }
        } else {
            // Ray is parallel to slab. Check if origin is outside box on this axis.
            if (ray.origin[i] < box.min[i] || ray.origin[i] > box.max[i]) {
                return false;
            }
        }
    }

    out_t_min = t_min;
    out_t_max = t_max;
    return true;
}

[[nodiscard]] inline bool intersect_ray_sphere(const Ray& ray, const Sphere& sphere, float& out_t1,
                                               float& out_t2) noexcept {
    Vec3 oc = ray.origin - sphere.center;
    float h = dot(ray.direction, oc);
    float c = dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = h * h - c;

    if (discriminant < 0.0f) {
        return false;
    }

    float sqrt_d = std::sqrt(discriminant);
    out_t1 = -h - sqrt_d;
    out_t2 = -h + sqrt_d;
    return true;
}

[[nodiscard]] constexpr bool intersect_aabb_aabb(const AABB& a, const AABB& b) noexcept {
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

[[nodiscard]] inline bool intersect_sphere_sphere(const Sphere& a, const Sphere& b) noexcept {
    float r_sum = a.radius + b.radius;
    return length_squared(a.center - b.center) <= r_sum * r_sum;
}

[[nodiscard]] inline bool intersect_sphere_aabb(const Sphere& s, const AABB& b) noexcept {
    // Find closest point on AABB to sphere center
    Vec3 closest(clamp(s.center.x, b.min.x, b.max.x), clamp(s.center.y, b.min.y, b.max.y),
                 clamp(s.center.z, b.min.z, b.max.z));
    return length_squared(closest - s.center) <= s.radius * s.radius;
}

} // namespace prism::math
