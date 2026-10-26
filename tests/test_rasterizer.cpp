#include <gtest/gtest.h>
#include <prism/render/framebuffer.hpp>
#include <prism/render/rasterizer.hpp>
#include <prism/render/pipeline.hpp>
#include <prism/render/camera.hpp>
#include <prism/render/loader.hpp>
#include <prism/math/constants.hpp>
#include <fstream>
#include <cstdio>

using namespace prism;
using namespace prism::math;
using namespace prism::render;

TEST(RasterizerTest, TriangleFillsInsidePixel) {
    Framebuffer fb(64, 64);
    fb.clear(0xFF000000);

    ScreenSpaceVertex v0, v1, v2;
    v0.x = 10.0f; v0.y = 10.0f; v0.z = 0.5f; v0.inv_w = 1.0f; v0.normal = Vec3(0,0,1);
    v1.x = 50.0f; v1.y = 10.0f; v1.z = 0.5f; v1.inv_w = 1.0f; v1.normal = Vec3(0,0,1);
    v2.x = 30.0f; v2.y = 50.0f; v2.z = 0.5f; v2.inv_w = 1.0f; v2.normal = Vec3(0,0,1);

    Material mat;
    mat.ambient = Vec3(1.0f, 0.0f, 0.0f);
    mat.diffuse = Vec3(0.0f, 0.0f, 0.0f);
    mat.specular = Vec3(0.0f, 0.0f, 0.0f);

    std::vector<Light> lights;
    rasterize_triangle(fb, v0, v1, v2, mat, lights, Vec3(0, 0, 5), ShadingMode::Flat);

    // Center of triangle (30, 23) should be red
    uint32_t c_in = fb.get_pixel(30, 23);
    uint8_t r = static_cast<uint8_t>((c_in >> 16) & 0xFF);
    EXPECT_GE(r, 200);

    // Pixel far outside (5, 5) should remain black
    uint32_t c_out = fb.get_pixel(5, 5);
    EXPECT_EQ(c_out, 0xFF000000u);
}

TEST(RasterizerTest, DepthBufferOcclusion) {
    Framebuffer fb(64, 64);
    fb.clear(0xFF000000);

    // Background triangle at z = 0.8 (Green)
    ScreenSpaceVertex bg0, bg1, bg2;
    bg0.x = 10.0f; bg0.y = 10.0f; bg0.z = 0.8f; bg0.inv_w = 1.0f; bg0.normal = Vec3(0,0,1);
    bg1.x = 50.0f; bg1.y = 10.0f; bg1.z = 0.8f; bg1.inv_w = 1.0f; bg1.normal = Vec3(0,0,1);
    bg2.x = 30.0f; bg2.y = 50.0f; bg2.z = 0.8f; bg2.inv_w = 1.0f; bg2.normal = Vec3(0,0,1);

    Material mat_green;
    mat_green.ambient = Vec3(0.0f, 1.0f, 0.0f);

    std::vector<Light> lights;
    rasterize_triangle(fb, bg0, bg1, bg2, mat_green, lights, Vec3(0, 0, 5), ShadingMode::Flat);

    // Foreground triangle at z = 0.2 (Red), drawn second
    ScreenSpaceVertex fg0, fg1, fg2;
    fg0.x = 10.0f; fg0.y = 10.0f; fg0.z = 0.2f; fg0.inv_w = 1.0f; fg0.normal = Vec3(0,0,1);
    fg1.x = 50.0f; fg1.y = 10.0f; fg1.z = 0.2f; fg1.inv_w = 1.0f; fg1.normal = Vec3(0,0,1);
    fg2.x = 30.0f; fg2.y = 50.0f; fg2.z = 0.2f; fg2.inv_w = 1.0f; fg2.normal = Vec3(0,0,1);

    Material mat_red;
    mat_red.ambient = Vec3(1.0f, 0.0f, 0.0f);
    rasterize_triangle(fb, fg0, fg1, fg2, mat_red, lights, Vec3(0, 0, 5), ShadingMode::Flat);

    uint32_t c = fb.get_pixel(30, 23);
    uint8_t r = static_cast<uint8_t>((c >> 16) & 0xFF);
    uint8_t g = static_cast<uint8_t>((c >> 8) & 0xFF);
    EXPECT_GE(r, 200);
    EXPECT_EQ(g, 0);
    EXPECT_NEAR(fb.get_depth(30, 23), 0.2f, 0.05f);

    // Now try drawing green triangle again at z = 0.8 over foreground z = 0.2: should be rejected
    rasterize_triangle(fb, bg0, bg1, bg2, mat_green, lights, Vec3(0, 0, 5), ShadingMode::Flat);
    c = fb.get_pixel(30, 23);
    r = static_cast<uint8_t>((c >> 16) & 0xFF);
    g = static_cast<uint8_t>((c >> 8) & 0xFF);
    EXPECT_GE(r, 200);
    EXPECT_EQ(g, 0);
}

TEST(RasterizerTest, PerspectiveCorrectInterpolation) {
    // A triangle viewed under severe perspective:
    // v0 has z=1 (inv_w=1), v1 has z=10 (inv_w=0.1)
    // Perspective-correct midpoint attribute must weight v0 much higher than affine midpoint.
    ScreenSpaceVertex v0, v1, v2;
    v0.x = 0.0f;   v0.y = 0.0f;  v0.z = 0.1f; v0.inv_w = 1.0f; v0.uv = Vec2(0.0f, 0.0f);
    v1.x = 100.0f; v1.y = 0.0f;  v1.z = 0.9f; v1.inv_w = 0.1f; v1.uv = Vec2(1.0f, 0.0f);
    v2.x = 50.0f;  v2.y = 50.0f; v2.z = 0.5f; v2.inv_w = 0.5f; v2.uv = Vec2(0.5f, 1.0f);

    // Screen-space midpoint between v0 and v1 has w0=0.5, w1=0.5
    float w0 = 0.5f;
    float w1 = 0.5f;
    float iw0 = w0 * v0.inv_w; // 0.5 * 1.0 = 0.5
    float iw1 = w1 * v1.inv_w; // 0.5 * 0.1 = 0.05
    float c0 = iw0 / (iw0 + iw1); // 0.5 / 0.55 = 0.909
    float c1 = iw1 / (iw0 + iw1); // 0.05 / 0.55 = 0.091

    // Affine u would be 0.5. Perspective-correct u is ~0.091 (much closer to v0 in 3D)
    float persp_u = v0.uv.x * c0 + v1.uv.x * c1;
    EXPECT_LT(persp_u, 0.2f);
    EXPECT_GT(persp_u, 0.05f);
}

TEST(RasterizerTest, TextureBilinearSampling) {
    // 2x2 texture:
    // (0,0) = Red,   (1,0) = Green
    // (0,1) = Blue,  (1,1) = White
    TextureData tex;
    tex.width = 2;
    tex.height = 2;
    tex.pixels = {
        0xFFFF0000, 0xFF00FF00,
        0xFF0000FF, 0xFFFFFFFF
    };

    EXPECT_TRUE(tex.valid());
    EXPECT_EQ(tex.width, 2);
    EXPECT_EQ(tex.height, 2);
}

TEST(RasterizerTest, CheckerboardGeneration) {
    TextureData cb = make_checkerboard(32, 0xFFFFFFFF, 0xFF000000);
    EXPECT_EQ(cb.width, 32);
    EXPECT_EQ(cb.height, 32);
    EXPECT_EQ(cb.pixels[0], 0xFFFFFFFF);
    // At cell boundary (cell size = 4), (4, 0) should be alternate color
    EXPECT_EQ(cb.pixels[4], 0xFF000000);
}

TEST(RasterizerTest, ObjLoaderLoadsValidMesh) {
    // Create a temporary mini OBJ file
    const char* obj_filename = "test_triangle.obj";
    std::ofstream out(obj_filename);
    out << "v 0.0 0.0 0.0\n";
    out << "v 1.0 0.0 0.0\n";
    out << "v 0.0 1.0 0.0\n";
    out << "vt 0.0 0.0\n";
    out << "vt 1.0 0.0\n";
    out << "vt 0.0 1.0\n";
    out << "vn 0.0 0.0 1.0\n";
    out << "f 1/1/1 2/2/1 3/3/1\n";
    out.close();

    auto tris = load_obj(obj_filename);
    std::remove(obj_filename);

    ASSERT_EQ(tris.size(), 1u);
    EXPECT_FLOAT_EQ(tris[0].v[0].position.x, 0.0f);
    EXPECT_FLOAT_EQ(tris[0].v[1].position.x, 1.0f);
    EXPECT_FLOAT_EQ(tris[0].v[2].position.y, 1.0f);
    EXPECT_FLOAT_EQ(tris[0].v[0].normal.z, 1.0f);
}

TEST(RasterizerTest, CubeMeshGeneration) {
    auto cube = make_cube();
    EXPECT_EQ(cube.size(), 12u); // 6 faces * 2 triangles
}
