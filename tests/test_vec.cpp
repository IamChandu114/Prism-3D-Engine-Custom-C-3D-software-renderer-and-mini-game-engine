#include <cmath>
#include <gtest/gtest.h>
#include <prism/math/vec2.hpp>
#include <prism/math/vec3.hpp>
#include <prism/math/vec4.hpp>

using namespace prism::math;

constexpr float kEpsilon = 1e-5f;

#define EXPECT_VEC2_NEAR(a, b, eps)                                                                \
    EXPECT_NEAR((a).x, (b).x, eps);                                                                \
    EXPECT_NEAR((a).y, (b).y, eps)

#define EXPECT_VEC3_NEAR(a, b, eps)                                                                \
    EXPECT_NEAR((a).x, (b).x, eps);                                                                \
    EXPECT_NEAR((a).y, (b).y, eps);                                                                \
    EXPECT_NEAR((a).z, (b).z, eps)

#define EXPECT_VEC4_NEAR(a, b, eps)                                                                \
    EXPECT_NEAR((a).x, (b).x, eps);                                                                \
    EXPECT_NEAR((a).y, (b).y, eps);                                                                \
    EXPECT_NEAR((a).z, (b).z, eps);                                                                \
    EXPECT_NEAR((a).w, (b).w, eps)

TEST(Vec2Test, BasicOperations) {
    Vec2 a{1.0f, 2.0f};
    Vec2 b{3.0f, 4.0f};

    EXPECT_VEC2_NEAR(a + b, Vec2(4.0f, 6.0f), kEpsilon);
    EXPECT_VEC2_NEAR(b - a, Vec2(2.0f, 2.0f), kEpsilon);
    EXPECT_VEC2_NEAR(a * 2.0f, Vec2(2.0f, 4.0f), kEpsilon);
    EXPECT_VEC2_NEAR(2.0f * a, Vec2(2.0f, 4.0f), kEpsilon);
    EXPECT_VEC2_NEAR(b / 2.0f, Vec2(1.5f, 2.0f), kEpsilon);
    EXPECT_VEC2_NEAR(-a, Vec2(-1.0f, -2.0f), kEpsilon);
}

TEST(Vec2Test, DotLengthNormalize) {
    Vec2 a{3.0f, 4.0f};
    EXPECT_NEAR(dot(a, a), 25.0f, kEpsilon);
    EXPECT_NEAR(length(a), 5.0f, kEpsilon);
    EXPECT_NEAR(length_squared(a), 25.0f, kEpsilon);

    Vec2 n = normalize(a);
    EXPECT_VEC2_NEAR(n, Vec2(0.6f, 0.8f), kEpsilon);
    EXPECT_NEAR(length(n), 1.0f, kEpsilon);
}

TEST(Vec2Test, NormalizeZeroVector) {
    Vec2 zero{0.0f, 0.0f};
    EXPECT_VEC2_NEAR(normalize(zero), Vec2(0.0f, 0.0f), kEpsilon);
}

TEST(Vec2Test, LerpOperations) {
    Vec2 a{0.0f, 0.0f};
    Vec2 b{10.0f, 20.0f};
    EXPECT_VEC2_NEAR(lerp(a, b, 0.0f), a, kEpsilon);
    EXPECT_VEC2_NEAR(lerp(a, b, 1.0f), b, kEpsilon);
    EXPECT_VEC2_NEAR(lerp(a, b, 0.5f), Vec2(5.0f, 10.0f), kEpsilon);
    EXPECT_VEC2_NEAR(lerp(a, b, -1.0f), Vec2(-10.0f, -20.0f), kEpsilon);
}

TEST(Vec2Test, ReflectOperations) {
    Vec2 v{1.0f, -1.0f};
    Vec2 n{0.0f, 1.0f}; // Flat horizontal surface pointing up
    EXPECT_VEC2_NEAR(reflect(v, n), Vec2(1.0f, 1.0f), kEpsilon);
}

TEST(Vec2Test, ProjectOperations) {
    Vec2 v{4.0f, 3.0f};
    Vec2 axis{1.0f, 0.0f};
    EXPECT_VEC2_NEAR(project(v, axis), Vec2(4.0f, 0.0f), kEpsilon);

    Vec2 zero{0.0f, 0.0f};
    EXPECT_VEC2_NEAR(project(v, zero), Vec2(0.0f, 0.0f), kEpsilon);
}

TEST(Vec3Test, BasicOperations) {
    Vec3 a{1.0f, 2.0f, 3.0f};
    Vec3 b{4.0f, 5.0f, 6.0f};

    EXPECT_VEC3_NEAR(a + b, Vec3(5.0f, 7.0f, 9.0f), kEpsilon);
    EXPECT_VEC3_NEAR(b - a, Vec3(3.0f, 3.0f, 3.0f), kEpsilon);
    EXPECT_VEC3_NEAR(a * 3.0f, Vec3(3.0f, 6.0f, 9.0f), kEpsilon);
    EXPECT_VEC3_NEAR(b / 2.0f, Vec3(2.0f, 2.5f, 3.0f), kEpsilon);
    EXPECT_VEC3_NEAR(-a, Vec3(-1.0f, -2.0f, -3.0f), kEpsilon);
}

TEST(Vec3Test, DotProduct) {
    Vec3 a{1.0f, 2.0f, 3.0f};
    Vec3 b{4.0f, -5.0f, 6.0f};
    EXPECT_NEAR(dot(a, b), 4.0f - 10.0f + 18.0f, kEpsilon);

    // Perpendicular vectors dot product is zero
    Vec3 x{1.0f, 0.0f, 0.0f};
    Vec3 y{0.0f, 1.0f, 0.0f};
    EXPECT_NEAR(dot(x, y), 0.0f, kEpsilon);
}

TEST(Vec3Test, CrossProductBasis) {
    Vec3 i{1.0f, 0.0f, 0.0f};
    Vec3 j{0.0f, 1.0f, 0.0f};
    Vec3 k{0.0f, 0.0f, 1.0f};

    EXPECT_VEC3_NEAR(cross(i, j), k, kEpsilon);
    EXPECT_VEC3_NEAR(cross(j, k), i, kEpsilon);
    EXPECT_VEC3_NEAR(cross(k, i), j, kEpsilon);
    EXPECT_VEC3_NEAR(cross(j, i), -k, kEpsilon);
}

TEST(Vec3Test, CrossProductParallelAndZero) {
    Vec3 p1{2.0f, 4.0f, 6.0f};
    Vec3 p2{-1.0f, -2.0f, -3.0f};
    EXPECT_VEC3_NEAR(cross(p1, p2), Vec3(0.0f, 0.0f, 0.0f), kEpsilon);
    EXPECT_VEC3_NEAR(cross(p1, p1), Vec3(0.0f, 0.0f, 0.0f), kEpsilon);
}

TEST(Vec3Test, LengthAndNormalize) {
    Vec3 v{2.0f, 3.0f, 6.0f};
    EXPECT_NEAR(length(v), 7.0f, kEpsilon);
    EXPECT_NEAR(length_squared(v), 49.0f, kEpsilon);

    Vec3 n = normalize(v);
    EXPECT_VEC3_NEAR(n, Vec3(2.0f / 7.0f, 3.0f / 7.0f, 6.0f / 7.0f), kEpsilon);
    EXPECT_NEAR(length(n), 1.0f, kEpsilon);
}

TEST(Vec3Test, NormalizeZeroVector) {
    Vec3 zero{0.0f, 0.0f, 0.0f};
    EXPECT_VEC3_NEAR(normalize(zero), Vec3(0.0f, 0.0f, 0.0f), kEpsilon);
}

TEST(Vec3Test, ProjectOperations) {
    Vec3 v{3.0f, 4.0f, 0.0f};
    Vec3 onto{1.0f, 0.0f, 0.0f};
    EXPECT_VEC3_NEAR(project(v, onto), Vec3(3.0f, 0.0f, 0.0f), kEpsilon);

    Vec3 zero{0.0f, 0.0f, 0.0f};
    EXPECT_VEC3_NEAR(project(v, zero), Vec3(0.0f, 0.0f, 0.0f), kEpsilon);
}

TEST(Vec3Test, ReflectOperations) {
    Vec3 ray{1.0f, -1.0f, 0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    EXPECT_VEC3_NEAR(reflect(ray, normal), Vec3(1.0f, 1.0f, 0.0f), kEpsilon);
}

TEST(Vec4Test, BasicOperations) {
    Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    Vec4 b{5.0f, 6.0f, 7.0f, 8.0f};

    EXPECT_VEC4_NEAR(a + b, Vec4(6.0f, 8.0f, 10.0f, 12.0f), kEpsilon);
    EXPECT_VEC4_NEAR(b - a, Vec4(4.0f, 4.0f, 4.0f, 4.0f), kEpsilon);
    EXPECT_VEC4_NEAR(a * 2.0f, Vec4(2.0f, 4.0f, 6.0f, 8.0f), kEpsilon);
}

TEST(Vec4Test, DotAndLength) {
    Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    Vec4 b{5.0f, 6.0f, 7.0f, 8.0f};
    EXPECT_NEAR(dot(a, b), 5.0f + 12.0f + 21.0f + 32.0f, kEpsilon);

    Vec4 v{0.0f, 3.0f, 4.0f, 0.0f};
    EXPECT_NEAR(length(v), 5.0f, kEpsilon);
}

TEST(Vec4Test, ConversionFromVec3) {
    Vec3 v3{1.0f, 2.0f, 3.0f};
    Vec4 point(v3, 1.0f);
    Vec4 direction(v3, 0.0f);
    EXPECT_VEC4_NEAR(point, Vec4(1.0f, 2.0f, 3.0f, 1.0f), kEpsilon);
    EXPECT_VEC4_NEAR(direction, Vec4(1.0f, 2.0f, 3.0f, 0.0f), kEpsilon);
    EXPECT_VEC3_NEAR(point.xyz(), v3, kEpsilon);
}
