#include <cmath>
#include <gtest/gtest.h>
#include <prism/render/simd_raster.hpp>

using namespace prism::render;

TEST(SimdTest, NumericalEquivalenceWithScalar) {
    float ax = 12.5f, ay = 45.2f;
    float bx = 89.1f, by = 120.7f;
    float py = 60.5f;
    float px0 = 10.0f, px1 = 35.0f, px2 = 60.0f, px3 = 85.0f;

    SimdEdge4 scalar_res = edge_function_4x_scalar(ax, ay, bx, by, px0, px1, px2, px3, py);
    SimdEdge4 simd_res = edge_function_4x_simd(ax, ay, bx, by, px0, px1, px2, px3, py);

    for (int i = 0; i < 4; ++i) {
        EXPECT_NEAR(simd_res.e[i], scalar_res.e[i], 1e-4f);
    }
}

TEST(SimdTest, MultiplePointsAndDirections) {
    // Triangle (0, 0) -> (100, 0) -> (50, 100)
    float ax = 0.0f, ay = 0.0f;
    float bx = 100.0f, by = 0.0f;

    float px0 = 20.0f, px1 = 40.0f, px2 = 60.0f, px3 = 80.0f;
    float py = 10.0f;

    SimdEdge4 scalar_res = edge_function_4x_scalar(ax, ay, bx, by, px0, px1, px2, px3, py);
    SimdEdge4 simd_res = edge_function_4x_simd(ax, ay, bx, by, px0, px1, px2, px3, py);

    for (int i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(simd_res.e[i], scalar_res.e[i]);
    }
}

#include <prism/math/simd.hpp>
#include <prism/math/transform.hpp>

TEST(SimdTest, Mat4Vec4SimdMatchesScalar) {
    using namespace prism::math;
    Mat4 m = translate(Vec3(3.2f, -1.8f, 7.5f)) * rotate_y(1.2f) * scale(Vec3(2.0f, 0.5f, 1.5f));
    Vec4 v(1.5f, -2.3f, 4.1f, 1.0f);

    Vec4 scalar_out = mat4_mul_vec4_scalar(m, v);
    Vec4 simd_out = mat4_mul_vec4_simd(m, v);

    EXPECT_NEAR(simd_out.x, scalar_out.x, 1e-5f);
    EXPECT_NEAR(simd_out.y, scalar_out.y, 1e-5f);
    EXPECT_NEAR(simd_out.z, scalar_out.z, 1e-5f);
    EXPECT_NEAR(simd_out.w, scalar_out.w, 1e-5f);
}
