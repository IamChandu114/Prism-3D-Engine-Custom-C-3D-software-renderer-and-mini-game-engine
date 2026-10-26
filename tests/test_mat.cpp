#include <gtest/gtest.h>
#include <prism/math/mat3.hpp>
#include <prism/math/mat4.hpp>
#include <prism/math/vec3.hpp>
#include <prism/math/vec4.hpp>

using namespace prism::math;

constexpr float kEpsilon = 1e-4f;

#define EXPECT_MAT3_NEAR(a, b, eps)                                                                \
    for (int col = 0; col < 3; ++col) {                                                            \
        for (int row = 0; row < 3; ++row) {                                                        \
            EXPECT_NEAR((a)(row, col), (b)(row, col), eps)                                         \
                << "Mismatch at row " << row << ", col " << col;                                   \
        }                                                                                          \
    }

#define EXPECT_MAT4_NEAR(a, b, eps)                                                                \
    for (int col = 0; col < 4; ++col) {                                                            \
        for (int row = 0; row < 4; ++row) {                                                        \
            EXPECT_NEAR((a)(row, col), (b)(row, col), eps)                                         \
                << "Mismatch at row " << row << ", col " << col;                                   \
        }                                                                                          \
    }

TEST(Mat3Test, IdentityMatrix) {
    Mat3 id = Mat3::identity();
    Vec3 v{3.0f, 4.0f, 5.0f};
    Vec3 transformed = id * v;
    EXPECT_NEAR(transformed.x, 3.0f, kEpsilon);
    EXPECT_NEAR(transformed.y, 4.0f, kEpsilon);
    EXPECT_NEAR(transformed.z, 5.0f, kEpsilon);
}

TEST(Mat3Test, TransposeMatrix) {
    Mat3 m{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f};
    Mat3 mt = transpose(m);
    EXPECT_NEAR(mt(0, 1), 4.0f, kEpsilon);
    EXPECT_NEAR(mt(1, 0), 2.0f, kEpsilon);
    EXPECT_NEAR(mt(0, 2), 7.0f, kEpsilon);
    EXPECT_NEAR(mt(2, 0), 3.0f, kEpsilon);
}

TEST(Mat3Test, TransposeMultiplicationProperty) {
    // (A * B)^T == B^T * A^T
    Mat3 a{1.0f, 2.0f, 0.0f, 0.0f, 1.0f, 1.0f, 2.0f, 0.0f, 1.0f};
    Mat3 b{0.0f, 1.0f, 2.0f, 1.0f, 2.0f, 0.0f, 1.0f, 0.0f, 1.0f};
    Mat3 ab_t = transpose(a * b);
    Mat3 bt_at = transpose(b) * transpose(a);
    EXPECT_MAT3_NEAR(ab_t, bt_at, kEpsilon);
}

TEST(Mat3Test, Determinant) {
    Mat3 m{2.0f, -1.0f, 0.0f, -1.0f, 2.0f, -1.0f, 0.0f, -1.0f, 2.0f};
    EXPECT_NEAR(determinant(m), 4.0f, kEpsilon);
}

TEST(Mat3Test, Inverse) {
    Mat3 m{2.0f, -1.0f, 0.0f, -1.0f, 2.0f, -1.0f, 0.0f, -1.0f, 2.0f};
    Mat3 inv = inverse(m);
    Mat3 prod = m * inv;
    EXPECT_MAT3_NEAR(prod, Mat3::identity(), kEpsilon);
}

TEST(Mat3Test, SingularMatrixInverseFallback) {
    Mat3 singular{1.0f, 2.0f, 3.0f, 2.0f, 4.0f, 6.0f, 0.0f, 1.0f, 0.0f};
    EXPECT_NEAR(determinant(singular), 0.0f, kEpsilon);
    Mat3 inv = inverse(singular);
    EXPECT_MAT3_NEAR(inv, Mat3::identity(), kEpsilon);
}

TEST(Mat4Test, IdentityAndVectorMultiply) {
    Mat4 id = Mat4::identity();
    Vec4 v{1.0f, 2.0f, 3.0f, 1.0f};
    Vec4 res = id * v;
    EXPECT_NEAR(res.x, 1.0f, kEpsilon);
    EXPECT_NEAR(res.y, 2.0f, kEpsilon);
    EXPECT_NEAR(res.z, 3.0f, kEpsilon);
    EXPECT_NEAR(res.w, 1.0f, kEpsilon);
}

TEST(Mat4Test, CompositionAssociativity) {
    Mat4 m1 = Mat4::identity();
    m1(0, 3) = 10.0f;
    Mat4 m2 = Mat4::identity();
    m2(1, 3) = 20.0f;

    Mat4 comp = m2 * m1;
    Vec4 v(0.0f, 0.0f, 0.0f, 1.0f);
    Vec4 res1 = comp * v;
    Vec4 res2 = m2 * (m1 * v);
    EXPECT_NEAR(res1.x, res2.x, kEpsilon);
    EXPECT_NEAR(res1.y, res2.y, kEpsilon);
    EXPECT_NEAR(res1.z, res2.z, kEpsilon);
}

TEST(Mat4Test, ColumnMajorMemoryLayout) {
    Mat4 m = Mat4::identity();
    m(1, 0) = 5.0f; // Row 1, Col 0 -> index = 0*4 + 1 = 1
    m(0, 1) = 9.0f; // Row 0, Col 1 -> index = 1*4 + 0 = 4

    const float* data = m.data();
    EXPECT_NEAR(data[1], 5.0f, kEpsilon);
    EXPECT_NEAR(data[4], 9.0f, kEpsilon);
}

TEST(Mat4Test, Transpose) {
    Mat4 m{1.0f, 2.0f,  3.0f,  4.0f,  5.0f,  6.0f,  7.0f,  8.0f,
           9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};
    Mat4 mt = transpose(m);
    EXPECT_NEAR(mt(0, 3), 13.0f, kEpsilon);
    EXPECT_NEAR(mt(3, 0), 4.0f, kEpsilon);
}

TEST(Mat4Test, GeneralDeterminantAndInverse) {
    Mat4 m{1.0f, 0.0f, 2.0f, -1.0f, 3.0f, 0.0f, 0.0f, 5.0f,
           2.0f, 1.0f, 4.0f, -3.0f, 1.0f, 0.0f, 5.0f, 0.0f};

    float det = determinant(m);
    EXPECT_NE(det, 0.0f);

    Mat4 inv = inverse(m);
    Mat4 id = m * inv;
    EXPECT_MAT4_NEAR(id, Mat4::identity(), kEpsilon);
}

TEST(Mat4Test, SingularMatrixDeterminantZero) {
    Mat4 singular{1.0f, 2.0f, 3.0f, 4.0f, 2.0f, 4.0f, 6.0f, 8.0f,
                  0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
    EXPECT_NEAR(determinant(singular), 0.0f, kEpsilon);
    Mat4 inv = inverse(singular);
    EXPECT_MAT4_NEAR(inv, Mat4::identity(), kEpsilon);
}

TEST(Mat4Test, AffineInverseFastPath) {
    Mat4 affine{0.0f, -1.0f, 0.0f, 3.0f, 1.0f, 0.0f, 0.0f, 4.0f,
                0.0f, 0.0f,  1.0f, 5.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    Mat4 inv_general = inverse(affine);
    Mat4 inv_fast = inverse_affine(affine);

    EXPECT_MAT4_NEAR(inv_fast, inv_general, kEpsilon);
    EXPECT_MAT4_NEAR(affine * inv_fast, Mat4::identity(), kEpsilon);
}
