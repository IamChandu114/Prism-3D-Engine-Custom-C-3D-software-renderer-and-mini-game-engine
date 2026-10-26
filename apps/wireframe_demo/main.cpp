// Stage 2 Demo: Spinning wireframe cube with FPS overlay
// Build: cmake --build build && ./build/apps/wireframe_demo [--headless N]
#include <prism/core/window.hpp>
#include <prism/math/constants.hpp>
#include <prism/math/quaternion.hpp>
#include <prism/math/transform.hpp>
#include <prism/render/camera.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/rasterizer.hpp>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

using namespace prism;
using namespace prism::math;
using namespace prism::render;

static constexpr int kWidth = 800;
static constexpr int kHeight = 600;

int main(int argc, char* argv[]) {
    bool headless = false;
    int headless_frames = 0;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--headless") == 0 && i + 1 < argc) {
            headless = true;
            headless_frames = std::atoi(argv[++i]);
        }
    }

    Framebuffer fb(kWidth, kHeight);

    Camera camera;
    camera.position = Vec3(0.0f, 0.5f, 3.0f);
    camera.aspect = static_cast<float>(kWidth) / kHeight;
    camera.fov_y = deg_to_rad(60.0f);
    camera.near_z = 0.1f;
    camera.far_z = 100.0f;

    auto cube_tris = make_cube();

    core::Window window("Prism – Wireframe Cube", kWidth, kHeight, headless);

    double angle = 0.0;
    int frame_count = 0;
    double fps = 0.0;
    double frame_ms = 0.0;

    auto t_prev = std::chrono::high_resolution_clock::now();
    auto t_fps_start = t_prev;
    int fps_frame_count = 0;

    while (window.is_running()) {
        if (headless && frame_count >= headless_frames)
            break;

        auto t_now = std::chrono::high_resolution_clock::now();
        double dt = std::chrono::duration<double>(t_now - t_prev).count();
        t_prev = t_now;
        frame_ms = dt * 1000.0;

        // FPS counter (update every second)
        ++fps_frame_count;
        double elapsed_fps = std::chrono::duration<double>(t_now - t_fps_start).count();
        if (elapsed_fps >= 1.0) {
            fps = fps_frame_count / elapsed_fps;
            fps_frame_count = 0;
            t_fps_start = t_now;
        }

        window.poll_events();

        // Spin cube 45 deg/sec around Y, 30 deg/sec around X
        angle += dt;
        Quat rot_y = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f),
                                           static_cast<float>(angle) * 45.0f * kPi / 180.0f);
        Quat rot_x = Quat::from_axis_angle(Vec3(1.0f, 0.0f, 0.0f),
                                           static_cast<float>(angle) * 30.0f * kPi / 180.0f);
        Mat4 model = (rot_y * rot_x).to_mat4();

        // Clear
        fb.clear(0xFF1A1A2E); // dark navy

        // Draw wireframe
        draw_wireframe(fb, cube_tris, model, camera, 0xFF00FFAA);

        // HUD overlay: FPS and frame time
        char buf[64];
        std::snprintf(buf, sizeof(buf), "FPS: %.0f  Frame: %.2f ms", fps, frame_ms);
        fb.draw_string(8, 8, buf, 0xFFFFFF00, 1);

        // Pipeline space label
        fb.draw_string(8, 20, "Pipeline: Model->World->View->Clip->NDC->Screen", 0xFF00CCFF, 1);

        if (headless) {
            char path[64];
            std::snprintf(path, sizeof(path), "frame_%04d.ppm", frame_count);
            (void)fb.save_ppm(path);
        } else {
            window.present(fb);
        }

        ++frame_count;
    }
    return 0;
}
