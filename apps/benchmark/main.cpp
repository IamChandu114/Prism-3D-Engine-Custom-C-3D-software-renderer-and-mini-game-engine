#include <prism/render/framebuffer.hpp>
#include <prism/render/rasterizer.hpp>
#include <prism/render/raster_opt.hpp>
#include <prism/render/camera.hpp>
#include <prism/math/transform.hpp>
#include <prism/math/constants.hpp>
#include <chrono>
#include <cstdio>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>

using namespace prism;
using namespace prism::math;
using namespace prism::render;

static constexpr int kWidth  = 640;
static constexpr int kHeight = 480;
static constexpr int kFrames = 60;

struct BenchmarkResult {
    std::string name;
    double total_ms;
    double avg_frame_ms;
    double fps;
    double speedup;
};

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  PRISM 3D SOFTWARE RENDERER - BENCHMARK SUITE          \n";
    std::cout << "  Resolution: " << kWidth << "x" << kHeight << " | Frames: " << kFrames << "\n";
    std::cout << "=========================================================\n\n";

    Framebuffer fb(kWidth, kHeight);

    Camera camera;
    camera.position = Vec3(0.0f, 2.0f, 8.0f);
    camera.aspect   = static_cast<float>(kWidth) / kHeight;
    camera.fov_y    = deg_to_rad(60.0f);
    camera.near_z   = 0.1f;
    camera.far_z    = 50.0f;

    auto cube_mesh = make_cube();

    Material mat;
    mat.ambient   = Vec3(0.1f, 0.1f, 0.15f);
    mat.diffuse   = Vec3(0.7f, 0.7f, 0.8f);
    mat.specular  = Vec3(0.5f, 0.5f, 0.5f);
    mat.shininess = 32.0f;

    std::vector<Light> lights(1);
    lights[0].type = LightType::Directional;
    lights[0].direction_or_position = normalize(Vec3(0.5f, -1.0f, -0.5f));
    lights[0].color = Vec3(1.0f, 1.0f, 1.0f);
    lights[0].intensity = 1.0f;

    RenderConfig config;
    config.shading = ShadingMode::Phong;
    config.backface_culling = true;
    config.depth_test = true;
    config.wireframe = false;

    // 27 cubes in a 3x3x3 grid
    std::vector<Vec3> cube_positions;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                cube_positions.push_back(Vec3(x * 2.5f, y * 2.5f, z * 2.5f));
            }
        }
    }

    std::vector<BenchmarkResult> results;
    double baseline_ms = 0.0;

    // ── Test 1: Baseline (Unoptimized, No Frustum Culling, Single Thread) ─────
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int f = 0; f < kFrames; ++f) {
            fb.clear(0xFF101015);
            for (const auto& pos : cube_positions) {
                Mat4 model = translate(pos) * scale(0.8f);
                draw_triangles(fb, cube_mesh, model, camera, mat, lights, config);
            }
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        baseline_ms = ms;
        results.push_back({"1. Baseline (Single-Thread, No Culling)", ms, ms / kFrames, (kFrames * 1000.0) / ms, 1.0});
    }

    // ── Test 2: + Frustum Culling ─────────────────────────────────────────────
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int f = 0; f < kFrames; ++f) {
            fb.clear(0xFF101015);
            Frustum frustum = Frustum::from_view_projection(camera.proj_matrix() * camera.view_matrix());
            for (const auto& pos : cube_positions) {
                AABB bounds(pos - Vec3(0.8f, 0.8f, 0.8f), pos + Vec3(0.8f, 0.8f, 0.8f));
                if (!is_visible(frustum, bounds)) continue;
                Mat4 model = translate(pos) * scale(0.8f);
                draw_triangles(fb, cube_mesh, model, camera, mat, lights, config);
            }
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        results.push_back({"+ Frustum Culling", ms, ms / kFrames, (kFrames * 1000.0) / ms, baseline_ms / ms});
    }

    // ── Test 3: + Batched Multithreaded Tiled Rasterizer (4 Threads) ────────
    {
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int f = 0; f < kFrames; ++f) {
            fb.clear(0xFF101015);
            Frustum frustum = Frustum::from_view_projection(camera.proj_matrix() * camera.view_matrix());
            std::vector<RenderMeshInstance> visible_instances;
            visible_instances.reserve(cube_positions.size());
            for (const auto& pos : cube_positions) {
                AABB bounds(pos - Vec3(0.8f, 0.8f, 0.8f), pos + Vec3(0.8f, 0.8f, 0.8f));
                if (!is_visible(frustum, bounds)) continue;
                RenderMeshInstance inst;
                inst.mesh = &cube_mesh;
                inst.model_matrix = translate(pos) * scale(0.8f);
                visible_instances.push_back(inst);
            }
            draw_scene_multithreaded(fb, visible_instances, camera, mat, lights, config, 4);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        results.push_back({"+ Batched Multithreaded (4 Threads)", ms, ms / kFrames, (kFrames * 1000.0) / ms, baseline_ms / ms});
    }

    // Print Formatted Markdown Table
    std::cout << "| Optimization Stage | Total Time (ms) | Frame Time (ms) | FPS | Speedup |\n";
    std::cout << "|:-------------------|----------------:|----------------:|----:|--------:|\n";
    for (const auto& r : results) {
        std::cout << "| " << std::left << std::setw(32) << r.name
                  << " | " << std::right << std::setw(15) << std::fixed << std::setprecision(1) << r.total_ms
                  << " | " << std::setw(15) << std::setprecision(2) << r.avg_frame_ms
                  << " | " << std::setw(5) << std::setprecision(1) << r.fps
                  << " | " << std::setw(6) << std::setprecision(2) << r.speedup << "x |\n";
    }

    // Output CSV
    std::ofstream csv("benchmark_results.csv");
    csv << "Stage,TotalTimeMs,FrameTimeMs,FPS,Speedup\n";
    for (const auto& r : results) {
        csv << "\"" << r.name << "\"," << r.total_ms << "," << r.avg_frame_ms << "," << r.fps << "," << r.speedup << "\n";
    }
    csv.close();

    std::cout << "\nResults saved to benchmark_results.csv\n";
    return 0;
}
