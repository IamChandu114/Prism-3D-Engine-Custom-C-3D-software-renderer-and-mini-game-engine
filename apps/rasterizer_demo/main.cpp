#include <prism/core/window.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/rasterizer.hpp>
#include <prism/render/camera.hpp>
#include <prism/render/loader.hpp>
#include <prism/math/quaternion.hpp>
#include <prism/math/transform.hpp>
#include <prism/math/constants.hpp>

#include <cstdio>
#include <string>
#include <chrono>
#include <cstring>
#include <cmath>

using namespace prism;
using namespace prism::math;
using namespace prism::render;

static constexpr int kWidth  = 800;
static constexpr int kHeight = 600;

int main(int argc, char* argv[]) {
    bool headless = false;
    int  headless_frames = 0;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--headless") == 0 && i + 1 < argc) {
            headless = true;
            headless_frames = std::atoi(argv[++i]);
        }
    }

    Framebuffer fb(kWidth, kHeight);

    Camera camera;
    camera.position = Vec3(0.0f, 1.0f, 3.2f);
    camera.aspect   = static_cast<float>(kWidth) / kHeight;
    camera.fov_y    = deg_to_rad(60.0f);
    camera.near_z   = 0.1f;
    camera.far_z    = 100.0f;

    auto mesh = make_cube();
    auto texture = make_checkerboard(256, 0xFFE0E0E0, 0xFF4040A0);

    Material mat;
    mat.ambient   = Vec3(0.08f, 0.08f, 0.12f);
    mat.diffuse   = Vec3(0.85f, 0.85f, 0.90f);
    mat.specular  = Vec3(0.9f, 0.9f, 0.9f);
    mat.shininess = 32.0f;
    mat.texture_data   = texture.pixels.data();
    mat.texture_width  = texture.width;
    mat.texture_height = texture.height;

    // Lights: 1 directional (sun) + 1 dynamic point light (orbiting fire)
    std::vector<Light> lights(2);
    lights[0].type = LightType::Directional;
    lights[0].direction_or_position = normalize(Vec3(0.5f, -1.0f, -0.6f));
    lights[0].color = Vec3(1.0f, 0.95f, 0.85f);
    lights[0].intensity = 0.8f;

    lights[1].type = LightType::Point;
    lights[1].color = Vec3(1.0f, 0.4f, 0.1f);
    lights[1].intensity = 2.0f;
    lights[1].radius = 4.0f;

    RenderConfig config;
    config.shading = ShadingMode::Phong;
    config.backface_culling = true;
    config.depth_test = true;
    config.wireframe = false;

    core::Window window("Prism – Stage 3 Software Rasterizer", kWidth, kHeight, headless);

    double sim_time = 0.0;
    int frame_count = 0;
    double fps = 0.0;
    double frame_ms = 0.0;

    auto t_prev = std::chrono::high_resolution_clock::now();
    auto t_fps_start = t_prev;
    int fps_frame_count = 0;

    while (window.is_running()) {
        if (headless && frame_count >= headless_frames) break;

        auto t_now = std::chrono::high_resolution_clock::now();
        double dt = std::chrono::duration<double>(t_now - t_prev).count();
        t_prev = t_now;
        frame_ms = dt * 1000.0;

        ++fps_frame_count;
        double elapsed_fps = std::chrono::duration<double>(t_now - t_fps_start).count();
        if (elapsed_fps >= 1.0) {
            fps = fps_frame_count / elapsed_fps;
            fps_frame_count = 0;
            t_fps_start = t_now;
        }

        window.poll_events();

        sim_time += dt;

        // Animate model rotation
        Quat rot_y = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), static_cast<float>(sim_time) * 0.8f);
        Quat rot_x = Quat::from_axis_angle(Vec3(1.0f, 0.0f, 0.0f), static_cast<float>(sim_time) * 0.4f);
        Mat4 model = (rot_y * rot_x).to_mat4();

        // Orbit dynamic point light
        float orbit_radius = 2.0f;
        lights[1].direction_or_position = Vec3(
            std::cos(static_cast<float>(sim_time) * 1.5f) * orbit_radius,
            0.5f + std::sin(static_cast<float>(sim_time) * 2.0f) * 0.5f,
            std::sin(static_cast<float>(sim_time) * 1.5f) * orbit_radius);

        fb.clear(0xFF141419);

        // Render solid rasterized mesh
        draw_triangles(fb, mesh, model, camera, mat, lights, config);

        // Overlay HUD
        char buf[128];
        std::snprintf(buf, sizeof(buf), "FPS: %.0f (%.2f ms) | Tris: %zu", fps, frame_ms, mesh.size());
        fb.draw_string(10, 10, buf, 0xFF00FFCC, 1);

        const char* shading_str = (config.shading == ShadingMode::Flat) ? "Flat" :
                                  (config.shading == ShadingMode::Gouraud) ? "Gouraud" : "Blinn-Phong";
        std::snprintf(buf, sizeof(buf), "Shading: %s | Culling: %s | Depth: %s",
                      shading_str, config.backface_culling ? "ON" : "OFF", config.depth_test ? "ON" : "OFF");
        fb.draw_string(10, 24, buf, 0xFFFFAA00, 1);

        fb.draw_string(10, 38, "Features: Sutherland-Hodgman Clip | Persp-Correct UV/Normal | Blinn-Phong", 0xFFAAAAAA, 1);

        if (headless) {
            char path[64];
            std::snprintf(path, sizeof(path), "raster_frame_%04d.ppm", frame_count);
            (void)fb.save_ppm(path);
        } else {
            window.present(fb);
        }

        ++frame_count;
    }

    return 0;
}
