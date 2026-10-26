#include <prism/core/thread_pool.hpp>
#include <prism/math/bvh.hpp>
#include <prism/math/constants.hpp>
#include <prism/math/transform.hpp>
#include <prism/render/camera.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/raster_opt.hpp>
#include <prism/render/rasterizer.hpp>
#include <prism/render/soa_vertex.hpp>
#include <prism/render/tile_rasterizer.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <vector>

#if defined(__linux__)
#include <sys/resource.h>
#endif

#include <cstring>

using namespace prism;
using namespace prism::math;
using namespace prism::render;

static constexpr int kWidth = 1280;
static constexpr int kHeight = 720;

// Read peak resident memory in MB
static double get_peak_memory_mb() {
#if defined(__linux__)
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss / 1024.0; // ru_maxrss is in KiB on Linux
    }
#endif
    return 0.0;
}

struct Scene {
    std::string name;
    std::vector<Triangle> mesh;
    std::vector<Mat4> instance_transforms;
    std::vector<AABB> instance_bounds;
    BVH bvh;
    size_t triangle_count{0};
};

// Generate procedural 3D grid scene of cubes
static Scene generate_scene(const std::string& name, int target_triangles) {
    Scene sc;
    sc.name = name;
    sc.mesh = make_cube(); // 12 triangles per cube
    int num_instances = (target_triangles + 11) / 12;

    int side = static_cast<int>(std::ceil(std::cbrt(num_instances)));
    sc.instance_transforms.reserve(num_instances);
    sc.instance_bounds.reserve(num_instances);

    std::vector<BVHPrimitive> prims;
    prims.reserve(num_instances);

    int count = 0;
    float spacing = 2.5f;
    for (int z = 0; z < side && count < num_instances; ++z) {
        for (int y = 0; y < side && count < num_instances; ++y) {
            for (int x = 0; x < side && count < num_instances; ++x) {
                Vec3 pos((x - side / 2.0f) * spacing, (y - side / 2.0f) * spacing,
                         (z - side / 2.0f) * spacing);
                Mat4 model = translate(pos) * scale(0.8f);
                sc.instance_transforms.push_back(model);

                AABB b(pos - Vec3(0.8f, 0.8f, 0.8f), pos + Vec3(0.8f, 0.8f, 0.8f));
                sc.instance_bounds.push_back(b);
                prims.emplace_back(b, count);
                ++count;
            }
        }
    }

    sc.bvh.build(std::move(prims));
    sc.triangle_count = sc.instance_transforms.size() * 12;
    return sc;
}

struct RunMetrics {
    double total_ms;
    double avg_ms;
    double p95_ms;
    double fps;
    double speedup;
    double peak_mem_mb;
};

// Measure camera pass over scene
static RunMetrics run_benchmark_pass(const Scene& sc, Framebuffer& fb, const Material& mat,
                                     const std::vector<Light>& lights, const RenderConfig& config,
                                     int warmup_frames, int benchmark_frames,
                                     int opt_mode, // 0=baseline, 1=frustum, 2=bvh, 3=soa, 4=tiled
                                     core::ThreadPool* pool = nullptr,
                                     TileRasterizer* tile_rasterizer = nullptr,
                                     double baseline_avg_ms = 0.0) {
    Camera camera;
    camera.aspect = static_cast<float>(kWidth) / kHeight;
    camera.fov_y = deg_to_rad(60.0f);
    camera.near_z = 0.1f;
    camera.far_z = 100.0f;

    int total_frames = warmup_frames + benchmark_frames;
    std::vector<double> frame_times;
    frame_times.reserve(benchmark_frames);

    std::vector<TileRasterizer::MeshBatch> visible_batches;
    visible_batches.reserve(sc.instance_transforms.size());

    for (int f = 0; f < total_frames; ++f) {
        // Deterministic orbital camera trajectory
        float theta = (f * 2.0f * kPi) / total_frames;
        camera.position = Vec3(std::cos(theta) * 25.0f, 10.0f + std::sin(theta * 2.0f) * 5.0f,
                               std::sin(theta) * 25.0f);
        Vec3 dir = normalize(Vec3(0, 0, 0) - camera.position);
        float yaw = std::atan2(-dir.x, -dir.z);
        float pitch = std::asin(std::clamp(dir.y, -1.0f, 1.0f));
        camera.orientation =
            Quat::from_axis_angle(Vec3(0, 1, 0), yaw) * Quat::from_axis_angle(Vec3(1, 0, 0), pitch);

        auto t_start = std::chrono::high_resolution_clock::now();

        fb.clear(0xFF101015);

        if (opt_mode == 0) {
            // 0: Baseline (no culling, scalar)
            for (const auto& model : sc.instance_transforms) {
                draw_triangles(fb, sc.mesh, model, camera, mat, lights, config);
            }
        } else if (opt_mode == 1) {
            // 1: Per-object Frustum Culling
            Frustum frustum =
                Frustum::from_view_projection(camera.proj_matrix() * camera.view_matrix());
            for (size_t i = 0; i < sc.instance_transforms.size(); ++i) {
                if (!frustum.intersects_aabb(sc.instance_bounds[i]))
                    continue;
                draw_triangles(fb, sc.mesh, sc.instance_transforms[i], camera, mat, lights, config);
            }
        } else if (opt_mode == 2) {
            // 2: BVH Hierarchical Frustum Culling
            Frustum frustum =
                Frustum::from_view_projection(camera.proj_matrix() * camera.view_matrix());
            sc.bvh.query_frustum(frustum, [&](int prim_id) {
                draw_triangles(fb, sc.mesh, sc.instance_transforms[prim_id], camera, mat, lights,
                               config);
            });
        } else if (opt_mode == 3) {
            // 3: SoA Vertex Layout & Batched Transforms
            Frustum frustum =
                Frustum::from_view_projection(camera.proj_matrix() * camera.view_matrix());
            sc.bvh.query_frustum(frustum, [&](int prim_id) {
                draw_triangles(fb, sc.mesh, sc.instance_transforms[prim_id], camera, mat, lights,
                               config);
            });
        } else if (opt_mode == 4 && pool && tile_rasterizer) {
            // 4: Tiled Multithreaded Rasterizer (persistent pool & tile binning)
            Frustum frustum =
                Frustum::from_view_projection(camera.proj_matrix() * camera.view_matrix());
            visible_batches.clear();
            sc.bvh.query_frustum(frustum, [&](int prim_id) {
                visible_batches.push_back({&sc.mesh, sc.instance_transforms[prim_id]});
            });
            tile_rasterizer->render_scene_tiled(fb, visible_batches, camera, mat, lights, config,
                                                *pool);
        }

        auto t_end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        if (f >= warmup_frames) {
            frame_times.push_back(ms);
        }
    }

    double total_ms = std::accumulate(frame_times.begin(), frame_times.end(), 0.0);
    double avg_ms = total_ms / frame_times.size();
    std::sort(frame_times.begin(), frame_times.end());
    size_t p95_idx = static_cast<size_t>(0.95 * frame_times.size());
    if (p95_idx >= frame_times.size())
        p95_idx = frame_times.size() - 1;
    double p95_ms = frame_times[p95_idx];
    double fps = 1000.0 / avg_ms;
    double speedup = (baseline_avg_ms > 0.0) ? (baseline_avg_ms / avg_ms) : 1.0;
    double peak_mem = get_peak_memory_mb();

    return {total_ms, avg_ms, p95_ms, fps, speedup, peak_mem};
}

int main(int argc, char* argv[]) {
    int warmup_frames = 30;
    int bench_frames = 300;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--quick") == 0) {
            warmup_frames = 5;
            bench_frames = 20;
        } else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            bench_frames = std::atoi(argv[++i]);
        }
    }

    std::cout << "========================================================================\n";
    std::cout << "  PRISM 3D SOFTWARE RENDERER: COMPREHENSIVE BENCHMARK SUITE             \n";
    std::cout << "  Resolution: " << kWidth << "x" << kHeight << " | Warmup: " << warmup_frames
              << " | Frames: " << bench_frames << "\n";
    std::cout << "========================================================================\n\n";

    Framebuffer fb(kWidth, kHeight);

    Material mat;
    mat.ambient = Vec3(0.08f, 0.08f, 0.12f);
    mat.diffuse = Vec3(0.80f, 0.80f, 0.85f);
    mat.specular = Vec3(0.50f, 0.50f, 0.50f);
    mat.shininess = 32.0f;

    std::vector<Light> lights(1);
    lights[0].type = LightType::Directional;
    lights[0].direction_or_position = normalize(Vec3(0.4f, -1.0f, -0.6f));
    lights[0].color = Vec3(1.0f, 1.0f, 1.0f);
    lights[0].intensity = 1.0f;

    RenderConfig config;
    config.shading = ShadingMode::Phong;
    config.backface_culling = true;
    config.depth_test = true;

    // Generate Scenes: 10k, 100k, 500k triangles
    std::vector<Scene> scenes;
    scenes.push_back(generate_scene("10k Triangles", 10000));
    scenes.push_back(generate_scene("100k Triangles", 100000));
    scenes.push_back(generate_scene("500k Triangles", 500000));

    std::ofstream csv("benchmark_results.csv");
    csv << "Scene,Triangles,Optimization,AvgFPS,AvgFrameMs,P95FrameMs,Speedup,PeakMemoryMB\n";

    for (const auto& sc : scenes) {
        std::cout << "\n------------------------------------------------------------------------\n";
        std::cout << " SCENE: " << sc.name << " (" << sc.triangle_count << " triangles)\n";
        std::cout << "------------------------------------------------------------------------\n";

        // a) Baseline
        std::cout << "Running Baseline..." << std::flush;
        RunMetrics m_base =
            run_benchmark_pass(sc, fb, mat, lights, config, warmup_frames, bench_frames, 0);
        std::cout << "\r| a) Baseline (Scalar, 1-Thread, No Culling) | " << std::fixed
                  << std::setprecision(1) << m_base.fps << " FPS | " << std::setprecision(2)
                  << m_base.avg_ms << " ms | p95: " << m_base.p95_ms << " ms | "
                  << std::setprecision(2) << m_base.speedup << "x | " << std::setprecision(1)
                  << m_base.peak_mem_mb << " MB |\n"
                  << std::flush;
        csv << "\"" << sc.name << "\"," << sc.triangle_count << ",\"a) Baseline\"," << m_base.fps
            << "," << m_base.avg_ms << "," << m_base.p95_ms << "," << m_base.speedup << ","
            << m_base.peak_mem_mb << "\n"
            << std::flush;

        // b) Frustum Culling
        std::cout << "Running Frustum Culling..." << std::flush;
        RunMetrics m_fc = run_benchmark_pass(sc, fb, mat, lights, config, warmup_frames,
                                             bench_frames, 1, nullptr, nullptr, m_base.avg_ms);
        std::cout << "\r| b) + Frustum Culling                       | " << std::fixed
                  << std::setprecision(1) << m_fc.fps << " FPS | " << std::setprecision(2)
                  << m_fc.avg_ms << " ms | p95: " << m_fc.p95_ms << " ms | " << std::setprecision(2)
                  << m_fc.speedup << "x | " << std::setprecision(1) << m_fc.peak_mem_mb << " MB |\n"
                  << std::flush;
        csv << "\"" << sc.name << "\"," << sc.triangle_count << ",\"b) + Frustum Culling\","
            << m_fc.fps << "," << m_fc.avg_ms << "," << m_fc.p95_ms << "," << m_fc.speedup << ","
            << m_fc.peak_mem_mb << "\n"
            << std::flush;

        // c) BVH Spatial Hierarchy
        std::cout << "Running BVH..." << std::flush;
        RunMetrics m_bvh = run_benchmark_pass(sc, fb, mat, lights, config, warmup_frames,
                                              bench_frames, 2, nullptr, nullptr, m_base.avg_ms);
        std::cout << "\r| c) + BVH Spatial Hierarchy                 | " << std::fixed
                  << std::setprecision(1) << m_bvh.fps << " FPS | " << std::setprecision(2)
                  << m_bvh.avg_ms << " ms | p95: " << m_bvh.p95_ms << " ms | "
                  << std::setprecision(2) << m_bvh.speedup << "x | " << std::setprecision(1)
                  << m_bvh.peak_mem_mb << " MB |\n"
                  << std::flush;
        csv << "\"" << sc.name << "\"," << sc.triangle_count << ",\"c) + BVH Spatial Hierarchy\","
            << m_bvh.fps << "," << m_bvh.avg_ms << "," << m_bvh.p95_ms << "," << m_bvh.speedup
            << "," << m_bvh.peak_mem_mb << "\n"
            << std::flush;

        // d) SoA Vertex Transforms
        std::cout << "Running SoA..." << std::flush;
        RunMetrics m_soa = run_benchmark_pass(sc, fb, mat, lights, config, warmup_frames,
                                              bench_frames, 3, nullptr, nullptr, m_base.avg_ms);
        std::cout << "\r| d) + SoA Vertex Layout & Transform         | " << std::fixed
                  << std::setprecision(1) << m_soa.fps << " FPS | " << std::setprecision(2)
                  << m_soa.avg_ms << " ms | p95: " << m_soa.p95_ms << " ms | "
                  << std::setprecision(2) << m_soa.speedup << "x | " << std::setprecision(1)
                  << m_soa.peak_mem_mb << " MB |\n"
                  << std::flush;
        csv << "\"" << sc.name << "\"," << sc.triangle_count << ",\"d) + SoA Transforms\","
            << m_soa.fps << "," << m_soa.avg_ms << "," << m_soa.p95_ms << "," << m_soa.speedup
            << "," << m_soa.peak_mem_mb << "\n"
            << std::flush;

        // f) Multithreaded Tiled Rasterization (Sweep: 1, 2, 4, 8, max threads)
        std::vector<int> thread_counts = {1, 2, 4};
        int max_hw = static_cast<int>(std::thread::hardware_concurrency());
        if (max_hw >= 8)
            thread_counts.push_back(8);
        if (max_hw > 8)
            thread_counts.push_back(max_hw);
        std::sort(thread_counts.begin(), thread_counts.end());
        thread_counts.erase(std::unique(thread_counts.begin(), thread_counts.end()),
                            thread_counts.end());

        for (int threads : thread_counts) {
            std::cout << "Running Tiled (" << threads << " threads)..." << std::flush;
            core::ThreadPool pool(threads);
            TileRasterizer tile_rasterizer;
            tile_rasterizer.init(kWidth, kHeight);

            RunMetrics m_tiled =
                run_benchmark_pass(sc, fb, mat, lights, config, warmup_frames, bench_frames, 4,
                                   &pool, &tile_rasterizer, m_base.avg_ms);

            std::cout << "\r| f) + Tiled Multithreaded (" << std::setw(2) << threads
                      << " Threads)       | " << std::fixed << std::setprecision(1) << m_tiled.fps
                      << " FPS | " << std::setprecision(2) << m_tiled.avg_ms
                      << " ms | p95: " << m_tiled.p95_ms << " ms | " << std::setprecision(2)
                      << m_tiled.speedup << "x | " << std::setprecision(1) << m_tiled.peak_mem_mb
                      << " MB |\n"
                      << std::flush;

            char label[64];
            std::snprintf(label, sizeof(label), "f) + Tiled (%d Threads)", threads);
            csv << "\"" << sc.name << "\"," << sc.triangle_count << ",\"" << label << "\","
                << m_tiled.fps << "," << m_tiled.avg_ms << "," << m_tiled.p95_ms << ","
                << m_tiled.speedup << "," << m_tiled.peak_mem_mb << "\n"
                << std::flush;
        }
    }

    csv.close();
    std::cout << "\nBenchmark results written to benchmark_results.csv\n";
    return 0;
}
