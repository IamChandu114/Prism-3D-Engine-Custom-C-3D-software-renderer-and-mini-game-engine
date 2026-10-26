#include <cmath>
#include <gtest/gtest.h>
#include <prism/math/constants.hpp>
#include <prism/math/mat4.hpp>
#include <prism/math/quaternion.hpp>
#include <prism/math/transform.hpp>

using namespace prism::math;

constexpr float kEpsilon = 1e-4f;

#define EXPECT_VEC3_NEAR(a, b, eps)                                                                \
    EXPECT_NEAR((a).x, (b).x, eps);                                                                \
    EXPECT_NEAR((a).y, (b).y, eps);                                                                \
    EXPECT_NEAR((a).z, (b).z, eps)

#define EXPECT_QUAT_NEAR(a, b, eps)                                                                \
    EXPECT_NEAR((a).w, (b).w, eps);                                                                \
    EXPECT_NEAR((a).x, (b).x, eps);                                                                \
    EXPECT_NEAR((a).y, (b).y, eps);                                                                \
    EXPECT_NEAR((a).z, (b).z, eps)

TEST(QuaternionTest, IdentityQuaternion) {
    Quat id = Quat::identity();
    EXPECT_QUAT_NEAR(id, Quat(1.0f, 0.0f, 0.0f, 0.0f), kEpsilon);
    EXPECT_NEAR(length(id), 1.0f, kEpsilon);
}

TEST(QuaternionTest, AxisAngleConstruction) {
    Quat q = Quat::from_axis_angle(Vec3(0.0f, 0.0f, 1.0f), kPi * 0.5f);
    float half = std::sqrt(0.5f);
    EXPECT_NEAR(q.w, half, kEpsilon);
    EXPECT_NEAR(q.x, 0.0f, kEpsilon);
    EXPECT_NEAR(q.y, 0.0f, kEpsilon);
    EXPECT_NEAR(q.z, half, kEpsilon);
}

TEST(QuaternionTest, RotateBasisVectors) {
    Quat q = Quat::from_axis_angle(Vec3(0.0f, 0.0f, 1.0f), kPi * 0.5f);
    Vec3 v(1.0f, 0.0f, 0.0f);
    Vec3 rot = q.rotate(v);
    EXPECT_VEC3_NEAR(rot, Vec3(0.0f, 1.0f, 0.0f), kEpsilon);
}

TEST(QuaternionTest, RotateArbitraryAxis) {
    Vec3 axis = normalize(Vec3(1.0f, 1.0f, 1.0f));
    Quat q_diag = Quat::from_axis_angle(axis, 2.0f * kPi / 3.0f);
    EXPECT_VEC3_NEAR(q_diag.rotate(Vec3(1.0f, 0.0f, 0.0f)), Vec3(0.0f, 1.0f, 0.0f), kEpsilon);
}

TEST(QuaternionTest, MultiplicationComposition) {
    Quat q1 = Quat::from_axis_angle(Vec3(1.0f, 0.0f, 0.0f), kPi * 0.5f);
    Quat q2 = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), kPi * 0.5f);

    Quat q_comp = q2 * q1;
    Vec3 v(0.0f, 1.0f, 0.0f);
    Vec3 step1 = q1.rotate(v);
    Vec3 step2 = q2.rotate(step1);
    EXPECT_VEC3_NEAR(q_comp.rotate(v), step2, kEpsilon);
}

TEST(QuaternionTest, ConjugateAndInverse) {
    Quat q = Quat::from_axis_angle(Vec3(1.0f, 2.0f, 3.0f), 0.7f);
    Quat inv = inverse(q);
    Quat conj = conjugate(q);
    EXPECT_QUAT_NEAR(inv, conj, kEpsilon);
    EXPECT_QUAT_NEAR(q * inv, Quat::identity(), kEpsilon);
}

TEST(QuaternionTest, ToMat4Equivalence) {
    Quat q = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), deg_to_rad(45.0f));
    Mat4 m = q.to_mat4();

    Vec3 v(1.0f, 2.0f, 3.0f);
    Vec3 v_quat = q.rotate(v);
    Vec4 v_mat = m * Vec4(v, 1.0f);

    EXPECT_VEC3_NEAR(v_quat, v_mat.xyz(), kEpsilon);
}

TEST(QuaternionTest, SlerpInterpolation) {
    Quat q0 = Quat::identity();
    Quat q1 = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), kPi * 0.5f);

    Quat mid_slerp = slerp(q0, q1, 0.5f);
    Quat mid_expected = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), kPi * 0.25f);
    EXPECT_QUAT_NEAR(mid_slerp, mid_expected, kEpsilon);
}

TEST(QuaternionTest, NlerpInterpolation) {
    Quat q0 = Quat::identity();
    Quat q1 = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), kPi * 0.5f);

    Quat mid_nlerp = nlerp(q0, q1, 0.5f);
    Quat mid_expected = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), kPi * 0.25f);
    EXPECT_QUAT_NEAR(mid_nlerp, mid_expected, 0.05f);
    EXPECT_NEAR(length(mid_nlerp), 1.0f, kEpsilon);
}

TEST(QuaternionTest, SlerpShortestPath) {
    Quat q0 = Quat::identity();
    Quat q1 = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), kPi * 0.5f);
    Quat q1_neg(-q1.w, -q1.x, -q1.y, -q1.z);

    Quat mid_expected = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), kPi * 0.25f);
    Quat mid_neg_slerp = slerp(q0, q1_neg, 0.5f);

    Vec3 v(1.0f, 0.0f, 0.0f);
    EXPECT_VEC3_NEAR(mid_neg_slerp.rotate(v), mid_expected.rotate(v), kEpsilon);
}

TEST(QuaternionTest, GimbalLockDemonstration) {
    float pitch = kPi * 0.5f; // 90 degrees
    float yaw1 = deg_to_rad(30.0f);
    float roll1 = deg_to_rad(0.0f);

    Mat4 eulerA = rotate_z(roll1) * rotate_x(pitch) * rotate_y(yaw1);

    float yaw2 = deg_to_rad(0.0f);
    float roll2 = deg_to_rad(30.0f);
    Mat4 eulerB = rotate_z(roll2) * rotate_x(pitch) * rotate_y(yaw2);

    Vec4 p(1.0f, 0.0f, 0.0f, 1.0f);
    Vec4 resA = eulerA * p;
    Vec4 resB = eulerB * p;
    EXPECT_NEAR(resA.z, resB.z, kEpsilon);

    Quat q_pitch = Quat::from_axis_angle(Vec3(1.0f, 0.0f, 0.0f), pitch);
    Quat q_yaw = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), yaw1);
    Quat q_roll = Quat::from_axis_angle(Vec3(0.0f, 0.0f, 1.0f), roll2);
    Quat q_combined = q_roll * q_pitch * q_yaw;

    EXPECT_NEAR(length(q_combined), 1.0f, kEpsilon);
}
