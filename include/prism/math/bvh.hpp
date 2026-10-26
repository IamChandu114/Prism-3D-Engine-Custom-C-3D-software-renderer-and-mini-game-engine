#pragma once

#include <algorithm>
#include <cstdint>
#include <prism/math/geometry.hpp>
#include <prism/math/intersection.hpp>
#include <vector>

namespace prism::math {

struct BVHPrimitive {
    AABB bounds;
    Vec3 center{0.0f, 0.0f, 0.0f};
    int id{0};

    BVHPrimitive() = default;
    BVHPrimitive(const AABB& b, int prim_id)
        : bounds(b), center((b.min + b.max) * 0.5f), id(prim_id) {}
};

struct BVHNode {
    AABB bounds;
    int left{-1};
    int right{-1};
    int first_prim{0};
    int prim_count{0};

    [[nodiscard]] bool is_leaf() const noexcept { return left == -1 && right == -1; }
};

class BVH {
public:
    std::vector<BVHNode> nodes;
    std::vector<BVHPrimitive> primitives;

    BVH() = default;

    void build(std::vector<BVHPrimitive> prims) {
        primitives = std::move(prims);
        nodes.clear();
        if (primitives.empty())
            return;

        nodes.reserve(primitives.size() * 2);
        build_recursive(0, static_cast<int>(primitives.size()));
    }

    [[nodiscard]] bool empty() const noexcept { return nodes.empty(); }

    template <typename Callback> void query_frustum(const Frustum& frustum, Callback&& cb) const {
        if (nodes.empty())
            return;
        query_frustum_recursive(0, frustum, cb);
    }

    template <typename Callback> void query_ray(const Ray& ray, Callback&& cb) const {
        if (nodes.empty())
            return;
        query_ray_recursive(0, ray, cb);
    }

    template <typename Callback> void query_sphere(const Sphere& sphere, Callback&& cb) const {
        if (nodes.empty())
            return;
        query_sphere_recursive(0, sphere, cb);
    }

private:
    int build_recursive(int start, int end) {
        int node_idx = static_cast<int>(nodes.size());
        nodes.emplace_back();

        // Compute enclosing bounding box
        AABB node_bounds = primitives[start].bounds;
        for (int i = start + 1; i < end; ++i) {
            node_bounds.expand(primitives[i].bounds);
        }
        nodes[node_idx].bounds = node_bounds;

        int count = end - start;
        if (count <= 2) {
            // Leaf node
            nodes[node_idx].first_prim = start;
            nodes[node_idx].prim_count = count;
            nodes[node_idx].left = -1;
            nodes[node_idx].right = -1;
            return node_idx;
        }

        // Split along the axis of maximum extent
        Vec3 extents = node_bounds.max - node_bounds.min;
        int axis = 0;
        if (extents.y > extents.x && extents.y > extents.z)
            axis = 1;
        else if (extents.z > extents.x)
            axis = 2;

        int mid = start + count / 2;
        std::nth_element(primitives.begin() + start, primitives.begin() + mid,
                         primitives.begin() + end,
                         [axis](const BVHPrimitive& a, const BVHPrimitive& b) {
                             if (axis == 0)
                                 return a.center.x < b.center.x;
                             if (axis == 1)
                                 return a.center.y < b.center.y;
                             return a.center.z < b.center.z;
                         });

        nodes[node_idx].first_prim = start;
        nodes[node_idx].prim_count = count;
        int left_child = build_recursive(start, mid);
        int right_child = build_recursive(mid, end);
        nodes[node_idx].left = left_child;
        nodes[node_idx].right = right_child;
        return node_idx;
    }

    template <typename Callback>
    void query_frustum_recursive(int node_idx, const Frustum& frustum, Callback& cb) const {
        const auto& node = nodes[node_idx];
        if (!frustum.intersects_aabb(node.bounds))
            return;

        if (node.is_leaf()) {
            for (int i = 0; i < node.prim_count; ++i) {
                const auto& prim = primitives[node.first_prim + i];
                if (frustum.intersects_aabb(prim.bounds)) {
                    cb(prim.id);
                }
            }
        } else {
            if (node.left != -1)
                query_frustum_recursive(node.left, frustum, cb);
            if (node.right != -1)
                query_frustum_recursive(node.right, frustum, cb);
        }
    }

    template <typename Callback>
    void query_ray_recursive(int node_idx, const Ray& ray, Callback& cb) const {
        const auto& node = nodes[node_idx];
        float t_min = 0.0f, t_max = 0.0f;
        if (!intersect_ray_aabb(ray, node.bounds, t_min, t_max))
            return;

        if (node.is_leaf()) {
            for (int i = 0; i < node.prim_count; ++i) {
                const auto& prim = primitives[node.first_prim + i];
                float pt0 = 0.0f, pt1 = 0.0f;
                if (intersect_ray_aabb(ray, prim.bounds, pt0, pt1)) {
                    cb(prim.id, pt0, pt1);
                }
            }
        } else {
            if (node.left != -1)
                query_ray_recursive(node.left, ray, cb);
            if (node.right != -1)
                query_ray_recursive(node.right, ray, cb);
        }
    }

    template <typename Callback>
    void query_sphere_recursive(int node_idx, const Sphere& sphere, Callback& cb) const {
        const auto& node = nodes[node_idx];
        if (!intersect_sphere_aabb(sphere, node.bounds))
            return;

        if (node.is_leaf()) {
            for (int i = 0; i < node.prim_count; ++i) {
                const auto& prim = primitives[node.first_prim + i];
                if (intersect_sphere_aabb(sphere, prim.bounds)) {
                    cb(prim.id);
                }
            }
        } else {
            if (node.left != -1)
                query_sphere_recursive(node.left, sphere, cb);
            if (node.right != -1)
                query_sphere_recursive(node.right, sphere, cb);
        }
    }
};

} // namespace prism::math
