#include <cmath>
#include <gtest/gtest.h>
#include <prism/math/constants.hpp>
#include <prism/math/geometry.hpp>
#include <prism/math/transform.hpp>

using namespace prism::math;

constexpr float kEpsilon = 1e-4f;

TEST(GeometryTest, BarycentricCentroid) {
    Vec2 a(0.0f, 0.0f);
    Vec2 b(1.0f, 0.0f);
    Vec2 c(0.0f, 1.0f);

    Vec2 centroid = (a + b + c) / 3.0f;
    Vec3 bary = barycentric(centroid, a, b, c);
    EXPECT_NEAR(bary.x, 1.0f / 3.0f, kEpsilon);
    EXPECT_NEAR(bary.y, 1.0f / 3.0f, kEpsilon);
    EXPECT_NEAR(bary.z, 1.0f / 3.0f, kEpsilon);
}

TEST(GeometryTest, BarycentricVertices) {
    Vec2 a(0.0f, 0.0f);
    Vec2 b(1.0f, 0.0f);
    Vec2 c(0.0f, 1.0f);

    Vec3 bary_a = barycentric(a, a, b, c);
    EXPECT_NEAR(bary_a.x, 1.0f, kEpsilon);
    EXPECT_NEAR(bary_a.y, 0.0f, kEpsilon);
    EXPECT_NEAR(bary_a.z, 0.0f, kEpsilon);

    Vec3 bary_b = barycentric(b, a, b, c);
    EXPECT_NEAR(bary_b.x, 0.0f, kEpsilon);
    EXPECT_NEAR(bary_b.y, 1.0f, kEpsilon);
    EXPECT_NEAR(bary_b.z, 0.0f, kEpsilon);

    Vec3 bary_c = barycentric(c, a, b, c);
    EXPECT_NEAR(bary_c.x, 0.0f, kEpsilon);
    EXPECT_NEAR(bary_c.y, 0.0f, kEpsilon);
    EXPECT_NEAR(bary_c.z, 1.0f, kEpsilon);
}

TEST(GeometryTest, BarycentricOutside) {
    Vec2 a(0.0f, 0.0f);
    Vec2 b(1.0f, 0.0f);
    Vec2 c(0.0f, 1.0f);

    Vec2 outside(2.0f, 2.0f);
    Vec3 bary_out = barycentric(outside, a, b, c);
    EXPECT_TRUE(bary_out.x < 0.0f || bary_out.y < 0.0f || bary_out.z < 0.0f);
}

TEST(GeometryTest, PlaneFromPoints) {
    Vec3 p0(0.0f, 0.0f, 0.0f);
    Vec3 p1(1.0f, 0.0f, 0.0f);
    Vec3 p2(0.0f, 1.0f, 0.0f);

    Plane pl = Plane::from_points(p0, p1, p2);
    EXPECT_NEAR(pl.normal.x, 0.0f, kEpsilon);
    EXPECT_NEAR(pl.normal.y, 0.0f, kEpsilon);
    EXPECT_NEAR(pl.normal.z, 1.0f, kEpsilon);
    EXPECT_NEAR(pl.distance, 0.0f, kEpsilon);
}

TEST(GeometryTest, PlaneSignedDistance) {
    Plane pl(Vec3(0.0f, 0.0f, 1.0f), 0.0f);

    Vec3 pt_above(0.0f, 0.0f, 5.0f);
    EXPECT_NEAR(pl.signed_distance(pt_above), 5.0f, kEpsilon);

    Vec3 pt_below(0.0f, 0.0f, -3.0f);
    EXPECT_NEAR(pl.signed_distance(pt_below), -3.0f, kEpsilon);
}

TEST(GeometryTest, AABBCenterAndExtents) {
    AABB box(Vec3(-1.0f, -2.0f, -3.0f), Vec3(1.0f, 2.0f, 3.0f));
    EXPECT_NEAR(box.center().x, 0.0f, kEpsilon);
    EXPECT_NEAR(box.center().y, 0.0f, kEpsilon);
    EXPECT_NEAR(box.center().z, 0.0f, kEpsilon);

    EXPECT_NEAR(box.extents().x, 1.0f, kEpsilon);
    EXPECT_NEAR(box.extents().y, 2.0f, kEpsilon);
    EXPECT_NEAR(box.extents().z, 3.0f, kEpsilon);
}

TEST(GeometryTest, AABBContainsPoint) {
    AABB box(Vec3(-1.0f, -1.0f, -1.0f), Vec3(1.0f, 1.0f, 1.0f));
    EXPECT_TRUE(box.contains(Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_TRUE(box.contains(Vec3(1.0f, 1.0f, 1.0f)));
    EXPECT_FALSE(box.contains(Vec3(1.1f, 0.0f, 0.0f)));
}

TEST(GeometryTest, AABBExpandPointAndAABB) {
    AABB box(Vec3(0.0f, 0.0f, 0.0f), Vec3(1.0f, 1.0f, 1.0f));
    box.expand(Vec3(2.0f, -1.0f, 0.5f));
    EXPECT_NEAR(box.min.y, -1.0f, kEpsilon);
    EXPECT_NEAR(box.max.x, 2.0f, kEpsilon);

    AABB box2(Vec3(5.0f, 5.0f, 5.0f), Vec3(6.0f, 6.0f, 6.0f));
    box.expand(box2);
    EXPECT_NEAR(box.max.x, 6.0f, kEpsilon);
    EXPECT_NEAR(box.max.y, 6.0f, kEpsilon);
    EXPECT_NEAR(box.max.z, 6.0f, kEpsilon);
}

TEST(GeometryTest, SphereContains) {
    Sphere s(Vec3(0.0f, 0.0f, 0.0f), 2.0f);
    EXPECT_TRUE(s.contains(Vec3(1.0f, 1.0f, 1.0f)));
    EXPECT_FALSE(s.contains(Vec3(2.0f, 2.0f, 0.0f)));
}

TEST(GeometryTest, RayPointAt) {
    Ray r(Vec3(1.0f, 2.0f, 3.0f), Vec3(0.0f, 0.0f, 1.0f));
    Vec3 p = r.point_at(10.0f);
    EXPECT_NEAR(p.x, 1.0f, kEpsilon);
    EXPECT_NEAR(p.y, 2.0f, kEpsilon);
    EXPECT_NEAR(p.z, 13.0f, kEpsilon);
}

TEST(GeometryTest, FrustumContainsPoint) {
    Mat4 view = look_at(Vec3(0.0f, 0.0f, 5.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    Mat4 proj = perspective(deg_to_rad(90.0f), 1.0f, 1.0f, 100.0f);
    Frustum frustum = Frustum::from_view_projection(proj * view);

    EXPECT_TRUE(frustum.contains_point(Vec3(0.0f, 0.0f, 0.0f)));
    EXPECT_FALSE(frustum.contains_point(Vec3(0.0f, 0.0f, 10.0f)));   // Behind camera
    EXPECT_FALSE(frustum.contains_point(Vec3(0.0f, 0.0f, -150.0f))); // Beyond far plane
}

TEST(GeometryTest, FrustumIntersectsSphere) {
    Mat4 view = look_at(Vec3(0.0f, 0.0f, 5.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    Mat4 proj = perspective(deg_to_rad(90.0f), 1.0f, 1.0f, 100.0f);
    Frustum frustum = Frustum::from_view_projection(proj * view);

    Sphere inside_sphere(Vec3(0.0f, 0.0f, 0.0f), 1.0f);
    EXPECT_TRUE(frustum.intersects_sphere(inside_sphere));

    Sphere behind_sphere(Vec3(0.0f, 0.0f, 20.0f), 1.0f);
    EXPECT_FALSE(frustum.intersects_sphere(behind_sphere));
}

TEST(GeometryTest, FrustumIntersectsAABB) {
    Mat4 view = look_at(Vec3(0.0f, 0.0f, 5.0f), Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));
    Mat4 proj = perspective(deg_to_rad(90.0f), 1.0f, 1.0f, 100.0f);
    Frustum frustum = Frustum::from_view_projection(proj * view);

    AABB inside_box(Vec3(-0.5f, -0.5f, -0.5f), Vec3(0.5f, 0.5f, 0.5f));
    EXPECT_TRUE(frustum.intersects_aabb(inside_box));

    AABB outside_box(Vec3(100.0f, 100.0f, 100.0f), Vec3(110.0f, 110.0f, 110.0f));
    EXPECT_FALSE(frustum.intersects_aabb(outside_box));
}
