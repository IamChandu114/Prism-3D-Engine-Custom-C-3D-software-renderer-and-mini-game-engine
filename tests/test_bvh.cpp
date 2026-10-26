#include <gtest/gtest.h>
#include <prism/math/bvh.hpp>
#include <prism/math/transform.hpp>
#include <random>
#include <set>

using namespace prism::math;

TEST(BVHTest, FrustumQueryMatchesBruteForce) {
    // Generate 100 random bounding boxes
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist_pos(-20.0f, 20.0f);
    std::uniform_real_distribution<float> dist_size(0.5f, 2.0f);

    std::vector<BVHPrimitive> prims;
    for (int i = 0; i < 100; ++i) {
        Vec3 pos(dist_pos(rng), dist_pos(rng), dist_pos(rng));
        Vec3 ext(dist_size(rng), dist_size(rng), dist_size(rng));
        prims.emplace_back(AABB(pos - ext, pos + ext), i);
    }

    BVH bvh;
    bvh.build(prims);

    // Create a perspective frustum looking down -Z
    Mat4 view = look_at(Vec3(0, 0, 10), Vec3(0, 0, 0), Vec3(0, 1, 0));
    Mat4 proj = perspective(deg_to_rad(60.0f), 16.0f / 9.0f, 0.1f, 30.0f);
    Frustum frustum = Frustum::from_view_projection(proj * view);

    // Brute force collection
    std::set<int> brute_force_ids;
    for (const auto& p : prims) {
        if (frustum.intersects_aabb(p.bounds)) {
            brute_force_ids.insert(p.id);
        }
    }

    // BVH query collection
    std::set<int> bvh_ids;
    bvh.query_frustum(frustum, [&](int id) { bvh_ids.insert(id); });

    EXPECT_EQ(bvh_ids, brute_force_ids);
}

TEST(BVHTest, RaycastQueryMatchesBruteForce) {
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist_pos(-10.0f, 10.0f);

    std::vector<BVHPrimitive> prims;
    for (int i = 0; i < 80; ++i) {
        Vec3 pos(dist_pos(rng), dist_pos(rng), dist_pos(rng));
        Vec3 ext(0.5f, 0.5f, 0.5f);
        prims.emplace_back(AABB(pos - ext, pos + ext), i);
    }

    BVH bvh;
    bvh.build(prims);

    Ray ray(Vec3(0, 0, 15), Vec3(0, 0, -1));

    // Brute force
    std::set<int> brute_hits;
    for (const auto& p : prims) {
        float t0 = 0.0f, t1 = 0.0f;
        if (intersect_ray_aabb(ray, p.bounds, t0, t1)) {
            brute_hits.insert(p.id);
        }
    }

    // BVH
    std::set<int> bvh_hits;
    bvh.query_ray(ray, [&](int id, float /*t0*/, float /*t1*/) { bvh_hits.insert(id); });

    EXPECT_EQ(bvh_hits, brute_hits);
}

TEST(BVHTest, SphereQueryMatchesBruteForce) {
    std::mt19937 rng(999);
    std::uniform_real_distribution<float> dist_pos(-15.0f, 15.0f);

    std::vector<BVHPrimitive> prims;
    for (int i = 0; i < 60; ++i) {
        Vec3 pos(dist_pos(rng), dist_pos(rng), dist_pos(rng));
        Vec3 ext(0.8f, 0.8f, 0.8f);
        prims.emplace_back(AABB(pos - ext, pos + ext), i);
    }

    BVH bvh;
    bvh.build(prims);

    Sphere sphere(Vec3(2.0f, 1.0f, 0.0f), 5.0f);

    // Brute force
    std::set<int> brute_overlaps;
    for (const auto& p : prims) {
        if (intersect_sphere_aabb(sphere, p.bounds)) {
            brute_overlaps.insert(p.id);
        }
    }

    // BVH
    std::set<int> bvh_overlaps;
    bvh.query_sphere(sphere, [&](int id) { bvh_overlaps.insert(id); });

    EXPECT_EQ(bvh_overlaps, brute_overlaps);
}
