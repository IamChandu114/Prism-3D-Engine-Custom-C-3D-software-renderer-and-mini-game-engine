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

// Global allocation tracker
static std::atomic<bool> g_track_allocations{false};
static std::atomic<size_t> g_allocation_count{0};

void* operator new(size_t size) {
    if (g_track_allocations.load(std::memory_order_relaxed)) {
        g_allocation_count.fetch_add(1, std::memory_order_relaxed);
    }
    void* p = std::malloc(size);
    if (!p)
        throw std::bad_alloc();
    return p;
}

void operator delete(void* p) noexcept {
    std::free(p);
}

void operator delete(void* p, size_t) noexcept {
    std::free(p);
}

using namespace prism;
using namespace prism::math;
using namespace prism::render;

TEST(MemoryTest, ZeroHeapAllocationsDuringRenderLoopAfterWarmup) {
    Framebuffer fb(320, 240);
    Camera camera;
    camera.position = Vec3(0, 0, 3);
    camera.aspect = 320.0f / 240.0f;

    auto mesh = make_cube();
    Material mat;
    std::vector<Light> lights;
    RenderConfig config;
    core::ThreadPool pool(2);

    TileRasterizer rasterizer;
    rasterizer.init(320, 240);

    TileRasterizer::MeshBatch batch{&mesh, Mat4::identity()};
    std::vector<TileRasterizer::MeshBatch> batches = {batch};

    // Warmup: run 2 frames to let persistent vectors reserve memory
    for (int i = 0; i < 2; ++i) {
        fb.clear(0xFF000000);
        rasterizer.render_scene_tiled(fb, batches, camera, mat, lights, config, pool);
    }

    // Now start tracking allocations
    g_allocation_count.store(0);
    g_track_allocations.store(true);

    // Render 5 consecutive frames
    for (int frame = 0; frame < 5; ++frame) {
        fb.clear(0xFF000000);
        rasterizer.render_scene_tiled(fb, batches, camera, mat, lights, config, pool);
    }

    g_track_allocations.store(false);

    // Assert: zero heap allocations in the steady-state render loop!
    EXPECT_EQ(g_allocation_count.load(), 0u);
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
