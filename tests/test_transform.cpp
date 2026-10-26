#include <cmath>
#include <gtest/gtest.h>
#include <prism/math/constants.hpp>
#include <prism/math/transform.hpp>

using namespace prism::math;

constexpr float kEpsilon = 1e-4f;

#define EXPECT_VEC4_NEAR(a, b, eps)                                                                \
    EXPECT_NEAR((a).x, (b).x, eps);                                                                \
    EXPECT_NEAR((a).y, (b).y, eps);                                                                \
    EXPECT_NEAR((a).z, (b).z, eps);                                                                \
    EXPECT_NEAR((a).w, (b).w, eps)

TEST(TransformTest, TranslationPoint) {
    Mat4 t = translate(Vec3(2.0f, -3.0f, 5.0f));
    Vec4 p(1.0f, 1.0f, 1.0f, 1.0f);
    EXPECT_VEC4_NEAR(t * p, Vec4(3.0f, -2.0f, 6.0f, 1.0f), kEpsilon);
}

TEST(TransformTest, TranslationVectorInvariant) {
    Mat4 t = translate(Vec3(2.0f, -3.0f, 5.0f));
    Vec4 dir(1.0f, 1.0f, 1.0f, 0.0f);
    EXPECT_VEC4_NEAR(t * dir, dir, kEpsilon);
}

TEST(TransformTest, NonUniformScaling) {
    Mat4 s = scale(Vec3(2.0f, 3.0f, 4.0f));
    Vec4 p(1.0f, 2.0f, 3.0f, 1.0f);
    EXPECT_VEC4_NEAR(s * p, Vec4(2.0f, 6.0f, 12.0f, 1.0f), kEpsilon);
}

TEST(TransformTest, UniformScaling) {
    Mat4 s = scale(2.5f);
    Vec4 p(2.0f, 4.0f, 6.0f, 1.0f);
    EXPECT_VEC4_NEAR(s * p, Vec4(5.0f, 10.0f, 15.0f, 1.0f), kEpsilon);
}

TEST(TransformTest, RotateX) {
    float pi_over_2 = kPi * 0.5f;
    Mat4 rx = rotate_x(pi_over_2);
    Vec4 py(0.0f, 1.0f, 0.0f, 1.0f);
    EXPECT_VEC4_NEAR(rx * py, Vec4(0.0f, 0.0f, 1.0f, 1.0f), kEpsilon);
}

TEST(TransformTest, RotateY) {
    float pi_over_2 = kPi * 0.5f;
    Mat4 ry = rotate_y(pi_over_2);
    Vec4 pz(0.0f, 0.0f, 1.0f, 1.0f);
    EXPECT_VEC4_NEAR(ry * pz, Vec4(1.0f, 0.0f, 0.0f, 1.0f), kEpsilon);
}

TEST(TransformTest, RotateZ) {
    float pi_over_2 = kPi * 0.5f;
    Mat4 rz = rotate_z(pi_over_2);
    Vec4 px(1.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_VEC4_NEAR(rz * px, Vec4(0.0f, 1.0f, 0.0f, 1.0f), kEpsilon);
}

TEST(TransformTest, RotateAxisAngle) {
    Vec3 axis = normalize(Vec3(1.0f, 1.0f, 1.0f));
    float angle = 2.0f * kPi / 3.0f; // 120 degrees around (1, 1, 1) cycles X -> Y -> Z -> X
    Mat4 r = rotate_axis_angle(axis, angle);

    Vec4 px(1.0f, 0.0f, 0.0f, 1.0f);
    EXPECT_VEC4_NEAR(r * px, Vec4(0.0f, 1.0f, 0.0f, 1.0f), kEpsilon);
}

TEST(TransformTest, LookAtEyeOrigin) {
    Vec3 eye(0.0f, 0.0f, 5.0f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);

    Mat4 view = look_at(eye, target, up);
    Vec4 eye_view = view * Vec4(eye, 1.0f);
    EXPECT_VEC4_NEAR(eye_view, Vec4(0.0f, 0.0f, 0.0f, 1.0f), kEpsilon);
}

TEST(TransformTest, LookAtTargetViewSpace) {
    Vec3 eye(0.0f, 0.0f, 5.0f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);

    Mat4 view = look_at(eye, target, up);
    Vec4 target_view = view * Vec4(target, 1.0f);
    EXPECT_VEC4_NEAR(target_view, Vec4(0.0f, 0.0f, -5.0f, 1.0f), kEpsilon);
}

TEST(TransformTest, PerspectiveNearAndFarPlanes) {
    float fov_y = deg_to_rad(90.0f);
    float aspect = 1.0f;
    float near_plane = 1.0f;
    float far_plane = 100.0f;

    Mat4 proj = perspective(fov_y, aspect, near_plane, far_plane);

    Vec4 p_near(0.0f, 0.0f, -1.0f, 1.0f);
    Vec4 clip_near = proj * p_near;
    Vec3 ndc_near = clip_near.xyz() / clip_near.w;
    EXPECT_NEAR(ndc_near.z, -1.0f, kEpsilon);

    Vec4 p_far(0.0f, 0.0f, -100.0f, 1.0f);
    Vec4 clip_far = proj * p_far;
    Vec3 ndc_far = clip_far.xyz() / clip_far.w;
    EXPECT_NEAR(ndc_far.z, 1.0f, kEpsilon);
}

TEST(TransformTest, PerspectiveFOVEdges) {
    float fov_y = deg_to_rad(90.0f);
    float aspect = 1.0f;
    Mat4 proj = perspective(fov_y, aspect, 1.0f, 100.0f);

    Vec4 corner(1.0f, 1.0f, -1.0f, 1.0f);
    Vec4 clip_corner = proj * corner;
    Vec3 ndc_corner = clip_corner.xyz() / clip_corner.w;
    EXPECT_NEAR(ndc_corner.x, 1.0f, kEpsilon);
    EXPECT_NEAR(ndc_corner.y, 1.0f, kEpsilon);
}

TEST(TransformTest, OrthographicProjection) {
    Mat4 ortho_mat = orthographic(-10.0f, 10.0f, -5.0f, 5.0f, 1.0f, 100.0f);

    Vec4 left_pt(-10.0f, 0.0f, -1.0f, 1.0f);
    Vec4 clip_left = ortho_mat * left_pt;
    EXPECT_NEAR(clip_left.x, -1.0f, kEpsilon);
    EXPECT_NEAR(clip_left.z, -1.0f, kEpsilon);

    Vec4 right_pt(10.0f, 5.0f, -100.0f, 1.0f);
    Vec4 clip_right = ortho_mat * right_pt;
    EXPECT_NEAR(clip_right.x, 1.0f, kEpsilon);
    EXPECT_NEAR(clip_right.y, 1.0f, kEpsilon);
    EXPECT_NEAR(clip_right.z, 1.0f, kEpsilon);
}
