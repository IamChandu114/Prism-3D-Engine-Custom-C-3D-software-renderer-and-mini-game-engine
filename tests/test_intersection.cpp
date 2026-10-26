#include <gtest/gtest.h>
#include <prism/math/intersection.hpp>

using namespace prism::math;

constexpr float kEpsilon = 1e-4f;

TEST(IntersectionTest, RayTriangleDirectHit) {
    Vec3 v0(-1.0f, -1.0f, 0.0f);
    Vec3 v1(1.0f, -1.0f, 0.0f);
    Vec3 v2(0.0f, 1.0f, 0.0f);

    Ray ray(Vec3(0.0f, 0.0f, 5.0f), Vec3(0.0f, 0.0f, -1.0f));
    RayTriangleHit hit;
    bool intersects = intersect_ray_triangle(ray, v0, v1, v2, hit);

    EXPECT_TRUE(intersects);
    EXPECT_NEAR(hit.t, 5.0f, kEpsilon);
    EXPECT_NEAR(hit.point.x, 0.0f, kEpsilon);
    EXPECT_NEAR(hit.point.y, 0.0f, kEpsilon);
    EXPECT_NEAR(hit.point.z, 0.0f, kEpsilon);
    EXPECT_GT(hit.u, 0.0f);
    EXPECT_GT(hit.v, 0.0f);
    EXPECT_LT(hit.u + hit.v, 1.0f);
}

TEST(IntersectionTest, RayTrianglePointingAway) {
    Vec3 v0(-1.0f, -1.0f, 0.0f);
    Vec3 v1(1.0f, -1.0f, 0.0f);
    Vec3 v2(0.0f, 1.0f, 0.0f);

    Ray ray_away(Vec3(0.0f, 0.0f, 5.0f), Vec3(0.0f, 0.0f, 1.0f));
    RayTriangleHit hit;
    EXPECT_FALSE(intersect_ray_triangle(ray_away, v0, v1, v2, hit));
}

TEST(IntersectionTest, RayTriangleParallel) {
    Vec3 v0(-1.0f, -1.0f, 0.0f);
    Vec3 v1(1.0f, -1.0f, 0.0f);
    Vec3 v2(0.0f, 1.0f, 0.0f);

    Ray ray_parallel(Vec3(0.0f, 0.0f, 1.0f), Vec3(1.0f, 0.0f, 0.0f));
    RayTriangleHit hit;
    EXPECT_FALSE(intersect_ray_triangle(ray_parallel, v0, v1, v2, hit));
}

TEST(IntersectionTest, RayTriangleMissOutside) {
    Vec3 v0(-1.0f, -1.0f, 0.0f);
    Vec3 v1(1.0f, -1.0f, 0.0f);
    Vec3 v2(0.0f, 1.0f, 0.0f);

    Ray ray_miss(Vec3(5.0f, 5.0f, 5.0f), Vec3(0.0f, 0.0f, -1.0f));
    RayTriangleHit hit;
    EXPECT_FALSE(intersect_ray_triangle(ray_miss, v0, v1, v2, hit));
}

TEST(IntersectionTest, RayAABBSlabFrontalHit) {
    AABB box(Vec3(-1.0f, -1.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f));
    Ray r(Vec3(0.0f, 0.0f, -5.0f), Vec3(0.0f, 0.0f, 1.0f));
    float t_min = 0.0f;
    float t_max = 0.0f;
    EXPECT_TRUE(intersect_ray_aabb(r, box, t_min, t_max));
    EXPECT_NEAR(t_min, 4.0f, kEpsilon);
    EXPECT_NEAR(t_max, 6.0f, kEpsilon);
}

TEST(IntersectionTest, RayAABBSlabInsideBox) {
    AABB box(Vec3(-1.0f, -1.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f));
    Ray r(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    float t_min = 0.0f;
    float t_max = 0.0f;
    EXPECT_TRUE(intersect_ray_aabb(r, box, t_min, t_max));
    EXPECT_NEAR(t_min, -1.0f, kEpsilon);
    EXPECT_NEAR(t_max, 1.0f, kEpsilon);
}

TEST(IntersectionTest, RayAABBSlabParallelOutside) {
    AABB box(Vec3(-1.0f, -1.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f));
    Ray r_parallel(Vec3(5.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    float t_min = 0.0f;
    float t_max = 0.0f;
    EXPECT_FALSE(intersect_ray_aabb(r_parallel, box, t_min, t_max));
}

TEST(IntersectionTest, RayAABBSlabMiss) {
    AABB box(Vec3(-1.0f, -1.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f));
    Ray r_miss(Vec3(2.0f, 2.0f, -5.0f), Vec3(0.0f, 0.0f, 1.0f));
    float t_min = 0.0f;
    float t_max = 0.0f;
    EXPECT_FALSE(intersect_ray_aabb(r_miss, box, t_min, t_max));
}

TEST(IntersectionTest, RaySphereTwoHits) {
    Sphere s(Vec3(0.0f, 0.0f, 0.0f), 2.0f);
    Ray r_through(Vec3(0.0f, 0.0f, -5.0f), Vec3(0.0f, 0.0f, 1.0f));
    float t1 = 0.0f;
    float t2 = 0.0f;
    EXPECT_TRUE(intersect_ray_sphere(r_through, s, t1, t2));
    EXPECT_NEAR(t1, 3.0f, kEpsilon);
    EXPECT_NEAR(t2, 7.0f, kEpsilon);
}

TEST(IntersectionTest, RaySphereMiss) {
    Sphere s(Vec3(0.0f, 0.0f, 0.0f), 2.0f);
    Ray r_miss(Vec3(0.0f, 3.0f, -5.0f), Vec3(0.0f, 0.0f, 1.0f));
    float t1 = 0.0f;
    float t2 = 0.0f;
    EXPECT_FALSE(intersect_ray_sphere(r_miss, s, t1, t2));
}

TEST(IntersectionTest, RaySphereTangent) {
    Sphere s(Vec3(0.0f, 0.0f, 0.0f), 2.0f);
    Ray r_tangent(Vec3(2.0f, 0.0f, -5.0f), Vec3(0.0f, 0.0f, 1.0f));
    float t1 = 0.0f;
    float t2 = 0.0f;
    EXPECT_TRUE(intersect_ray_sphere(r_tangent, s, t1, t2));
    EXPECT_NEAR(t1, 5.0f, kEpsilon);
    EXPECT_NEAR(t2, 5.0f, kEpsilon);
}

TEST(IntersectionTest, AABBAABBOverlapping) {
    AABB b1(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    AABB b2(Vec3(1.0f, 1.0f, 1.0f), Vec3(3.0f, 3.0f, 3.0f));
    EXPECT_TRUE(intersect_aabb_aabb(b1, b2));
}

TEST(IntersectionTest, AABBAABBDisjoint) {
    AABB b1(Vec3(0.0f, 0.0f, 0.0f), Vec3(2.0f, 2.0f, 2.0f));
    AABB b3(Vec3(3.0f, 3.0f, 3.0f), Vec3(4.0f, 4.0f, 4.0f));
    EXPECT_FALSE(intersect_aabb_aabb(b1, b3));
}

TEST(IntersectionTest, SphereSphereOverlapping) {
    Sphere s1(Vec3(0.0f, 0.0f, 0.0f), 1.0f);
    Sphere s2(Vec3(1.5f, 0.0f, 0.0f), 1.0f);
    EXPECT_TRUE(intersect_sphere_sphere(s1, s2));
}

TEST(IntersectionTest, SphereSphereDisjoint) {
    Sphere s1(Vec3(0.0f, 0.0f, 0.0f), 1.0f);
    Sphere s3(Vec3(3.0f, 0.0f, 0.0f), 1.0f);
    EXPECT_FALSE(intersect_sphere_sphere(s1, s3));
}

TEST(IntersectionTest, SphereAABBOverlapping) {
    AABB box(Vec3(-1.0f, -1.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f));
    Sphere s_overlap(Vec3(1.5f, 0.0f, 0.0f), 1.0f);
    EXPECT_TRUE(intersect_sphere_aabb(s_overlap, box));
}

TEST(IntersectionTest, SphereAABBDisjoint) {
    AABB box(Vec3(-1.0f, -1.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f));
    Sphere s_disjoint(Vec3(3.0f, 0.0f, 0.0f), 0.5f);
    EXPECT_FALSE(intersect_sphere_aabb(s_disjoint, box));
}
