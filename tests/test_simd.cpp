#include <gtest/gtest.h>
#include <prism/render/simd_raster.hpp>
#include <cmath>

using namespace prism::render;

TEST(SimdTest, NumericalEquivalenceWithScalar) {
    float ax = 12.5f, ay = 45.2f;
    float bx = 89.1f, by = 120.7f;
    float py = 60.5f;
    float px0 = 10.0f, px1 = 35.0f, px2 = 60.0f, px3 = 85.0f;

    SimdEdge4 scalar_res = edge_function_4x_scalar(ax, ay, bx, by, px0, px1, px2, px3, py);
    SimdEdge4 simd_res   = edge_function_4x_simd(ax, ay, bx, by, px0, px1, px2, px3, py);

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
    SimdEdge4 simd_res   = edge_function_4x_simd(ax, ay, bx, by, px0, px1, px2, px3, py);

    for (int i = 0; i < 4; ++i) {
        EXPECT_FLOAT_EQ(simd_res.e[i], scalar_res.e[i]);
    }
}
