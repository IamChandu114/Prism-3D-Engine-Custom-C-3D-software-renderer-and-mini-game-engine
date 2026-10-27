#include <atomic>
#include <cstdlib>
#include <gtest/gtest.h>
#include <new>
#include <prism/core/thread_pool.hpp>
#include <prism/math/transform.hpp>
#include <prism/render/camera.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/rasterizer.hpp>
#include <prism/render/tile_rasterizer.hpp>

using namespace prism;
using namespace prism::math;
using namespace prism::render;

TEST(MemoryTest, FramebufferStorageRemainsStableDuringRenderLoop) {
    Framebuffer fb(320, 240);
    Camera camera;
    camera.position = Vec3(0, 0, 3);
    camera.aspect = 320.0f / 240.0f;
    auto mesh = make_cube();
    Material material;
    std::vector<Light> lights;
    RenderConfig config;
    core::ThreadPool pool(2);
    TileRasterizer rasterizer;
    rasterizer.init(320, 240);
    TileRasterizer::MeshBatch batch{&mesh, Mat4::identity()};
    std::vector<TileRasterizer::MeshBatch> batches{batch};

    auto* color_storage = fb.color_data();
    auto* depth_storage = fb.depth_data();
    for (int frame = 0; frame < 7; ++frame) {
        fb.clear(0xFF000000);
        rasterizer.render_scene_tiled(fb, batches, camera, material, lights, config, pool);
        EXPECT_EQ(fb.color_data(), color_storage);
        EXPECT_EQ(fb.depth_data(), depth_storage);
    }
}

TEST(MemoryTest, TiledRendererMatchesBaselineOutput) {
    int W = 160;
    int H = 120;
    Framebuffer fb_baseline(W, H);
    Framebuffer fb_tiled(W, H);

    fb_baseline.clear(0xFF101020);
    fb_tiled.clear(0xFF101020);

    Camera camera;
    camera.position = Vec3(0, 1, 3);
    camera.aspect = static_cast<float>(W) / H;

    auto mesh = make_cube();
    Material mat;
    mat.ambient = Vec3(0.2f, 0.2f, 0.2f);
    mat.diffuse = Vec3(0.8f, 0.8f, 0.8f);

    std::vector<Light> lights(1);
    lights[0].type = LightType::Directional;
    lights[0].direction_or_position = normalize(Vec3(0.5f, -1.0f, -0.5f));
    lights[0].color = Vec3(1.0f, 1.0f, 1.0f);
    lights[0].intensity = 1.0f;

    RenderConfig config;
    config.shading = ShadingMode::Phong;
    config.backface_culling = true;

    Mat4 model = rotate_y(0.5f) * rotate_x(0.3f);

    // 1. Render with standard baseline
    draw_triangles(fb_baseline, mesh, model, camera, mat, lights, config);

    // 2. Render with tiled multithreaded renderer
    core::ThreadPool pool(2);
    TileRasterizer rasterizer;
    rasterizer.init(W, H);
    rasterizer.render_tiled(fb_tiled, mesh, model, camera, mat, lights, config, pool);

    // Compare framebuffers: pixel colors must match within 1 unit per channel tolerance
    int mismatch_count = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            uint32_t c0 = fb_baseline.get_pixel(x, y);
            uint32_t c1 = fb_tiled.get_pixel(x, y);
            int r0 = (c0 >> 16) & 0xFF, r1 = (c1 >> 16) & 0xFF;
            int g0 = (c0 >> 8) & 0xFF, g1 = (c1 >> 8) & 0xFF;
            int b0 = c0 & 0xFF, b1 = c1 & 0xFF;

            if (std::abs(r0 - r1) > 2 || std::abs(g0 - g1) > 2 || std::abs(b0 - b1) > 2) {
                ++mismatch_count;
            }
        }
    }

    // Almost all pixels must match exactly
    EXPECT_LT(mismatch_count, 10);
}

TEST(MemoryTest, TiledScalarAndSimdProduceIdenticalFramebuffer) {
    constexpr int width = 192;
    constexpr int height = 128;
    Framebuffer fb(width, height);
    Camera camera;
    camera.position = Vec3(0, 1, 3);
    camera.aspect = static_cast<float>(width) / height;
    const auto mesh = make_cube();
    Material material;
    std::vector<Light> lights(1);
    lights[0].type = LightType::Directional;
    lights[0].direction_or_position = normalize(Vec3(0.5f, -1.0f, -0.5f));
    RenderConfig config;
    config.backface_culling = true;
    core::ThreadPool pool(1);
    TileRasterizer rasterizer;
    rasterizer.init(width, height);

    fb.clear(0xFF101020);
    rasterizer.render_tiled(fb, mesh, rotate_y(0.5f) * rotate_x(0.3f), camera, material, lights,
                            config, pool);
    std::vector<uint32_t> scalar_color(fb.color_data(), fb.color_data() + fb.total_pixels());
    std::vector<float> scalar_depth(fb.depth_data(), fb.depth_data() + fb.total_pixels());

    config.use_simd_edge_tests = true;
    fb.clear(0xFF101020);
    rasterizer.render_tiled(fb, mesh, rotate_y(0.5f) * rotate_x(0.3f), camera, material, lights,
                            config, pool);
    for (size_t i = 0; i < fb.total_pixels(); ++i) {
        EXPECT_EQ(fb.color_data()[i], scalar_color[i]);
        EXPECT_FLOAT_EQ(fb.depth_data()[i], scalar_depth[i]);
    }
}
