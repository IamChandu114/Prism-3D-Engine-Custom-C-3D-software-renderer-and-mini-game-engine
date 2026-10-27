#include <prism/core/window.hpp>
#include <prism/math/constants.hpp>
#include <prism/math/quaternion.hpp>
#include <prism/math/transform.hpp>
#include <prism/render/camera.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/loader.hpp>
#include <prism/render/postprocess.hpp>
#include <prism/render/rasterizer.hpp>

#if !defined(PRISM_HEADLESS)
#include <SDL2/SDL.h>
#endif

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

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
    camera.position = Vec3(0.0f, 1.2f, 3.4f);
    camera.aspect = static_cast<float>(kWidth) / kHeight;
    camera.fov_y = deg_to_rad(55.0f);
    camera.near_z = 0.1f;
    camera.far_z = 100.0f;

    auto mesh_cube = make_cube();

    // Procedural High-Detail Textures
    std::vector<TextureData> textures;
    textures.push_back(make_scifi_metal_texture(256));
    textures.push_back(make_glowing_crystal_texture(256));
    textures.push_back(make_stone_wall_texture(256));
    textures.push_back(make_checkerboard(256, 0xFFE0E0E0, 0xFF4040A0));
    int current_tex_idx = 0;

    Material mat;
    mat.ambient = Vec3(0.12f, 0.12f, 0.16f);
    mat.diffuse = Vec3(0.85f, 0.85f, 0.90f);
    mat.specular = Vec3(1.0f, 1.0f, 1.0f);
    mat.shininess = 64.0f;
    mat.texture_data = textures[0].pixels.data();
    mat.texture_width = textures[0].width;
    mat.texture_height = textures[0].height;

    // Pedestal floor material
    auto floor_tex = make_flagstone_floor_texture(256);
    Material pedestal_mat;
    pedestal_mat.ambient = Vec3(0.08f, 0.08f, 0.10f);
    pedestal_mat.diffuse = Vec3(0.6f, 0.6f, 0.65f);
    pedestal_mat.specular = Vec3(0.4f, 0.4f, 0.4f);
    pedestal_mat.shininess = 32.0f;
    pedestal_mat.texture_data = floor_tex.pixels.data();
    pedestal_mat.texture_width = floor_tex.width;
    pedestal_mat.texture_height = floor_tex.height;

    // Light marker material
    Material orb_mat;
    orb_mat.ambient = Vec3(0.2f, 0.9f, 1.0f);
    orb_mat.diffuse = Vec3(0.3f, 0.9f, 1.0f);
    orb_mat.specular = Vec3(1.0f, 1.0f, 1.0f);
    orb_mat.shininess = 128.0f;

    // 3-Point Studio Lighting: Key + Orbiting Fill + Rim Light
    std::vector<Light> lights(3);
    // 1. Warm Key Sun
    lights[0].type = LightType::Directional;
    lights[0].direction_or_position = normalize(Vec3(0.6f, -1.0f, -0.7f));
    lights[0].color = Vec3(1.0f, 0.92f, 0.82f);
    lights[0].intensity = 0.95f;

    // 2. Dynamic Orbiting Cyan Fill Light
    lights[1].type = LightType::Point;
    lights[1].color = Vec3(0.2f, 0.85f, 1.0f);
    lights[1].intensity = 2.4f;
    lights[1].radius = 4.5f;

    // 3. Vibrant Magenta Rim Backlight
    lights[2].type = LightType::Point;
    lights[2].direction_or_position = Vec3(-1.8f, 1.5f, -1.8f);
    lights[2].color = Vec3(1.0f, 0.25f, 0.8f);
    lights[2].intensity = 1.8f;
    lights[2].radius = 5.0f;

    RenderConfig config;
    config.shading = ShadingMode::Phong;
    config.backface_culling = true;
    config.depth_test = true;
    config.wireframe = false;

    // Post-Processing Setup
    PostProcessor post_processor;
    post_processor.init(kWidth, kHeight);
    PostProcessConfig post_cfg;
    post_cfg.enable_fog = false; // studio backdrop
    post_cfg.enable_bloom = true;
    post_cfg.bloom_threshold = 0.62f;
    post_cfg.bloom_intensity = 0.55f;
    post_cfg.enable_vignette = true;
    post_cfg.vignette_radius = 0.72f;
    post_cfg.vignette_softness = 0.45f;
    post_cfg.enable_color_grade = true;
    post_cfg.contrast = 1.15f;
    post_cfg.saturation = 1.12f;
    post_cfg.exposure = 1.06f;

    bool enable_post_process = true;
    bool enable_texture = true;

    core::Window window("Prism – Stage 3 Visual Effects & 3D Rasterizer", kWidth, kHeight,
                        headless);

    double sim_time = 0.0;
    int frame_count = 0;
    double fps = 0.0;
    double frame_ms = 0.0;

    auto t_prev = std::chrono::high_resolution_clock::now();
    auto t_fps_start = t_prev;
    int fps_frame_count = 0;

#if !defined(PRISM_HEADLESS)
    // Key press debounce flags are only used by the interactive SDL path.
    bool key_1_down = false, key_2_down = false, key_3_down = false;
    bool key_t_down = false, key_b_down = false, key_p_down = false, key_m_down = false;
#endif

    while (window.is_running()) {
        if (headless && frame_count >= headless_frames)
            break;

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

#if !defined(PRISM_HEADLESS)
        const uint8_t* keys = SDL_GetKeyboardState(nullptr);
        if (keys) {
            // Shading toggles
            if (keys[SDL_SCANCODE_1] && !key_1_down)
                config.shading = ShadingMode::Flat;
            if (keys[SDL_SCANCODE_2] && !key_2_down)
                config.shading = ShadingMode::Gouraud;
            if (keys[SDL_SCANCODE_3] && !key_3_down)
                config.shading = ShadingMode::Phong;

            // Texture toggle
            if (keys[SDL_SCANCODE_T] && !key_t_down) {
                enable_texture = !enable_texture;
            }
            // Material cycle
            if (keys[SDL_SCANCODE_M] && !key_m_down) {
                current_tex_idx = (current_tex_idx + 1) % textures.size();
            }
            // Backface culling toggle
            if (keys[SDL_SCANCODE_B] && !key_b_down) {
                config.backface_culling = !config.backface_culling;
            }
            // Post-processing toggle
            if (keys[SDL_SCANCODE_P] && !key_p_down) {
                enable_post_process = !enable_post_process;
            }

            key_1_down = keys[SDL_SCANCODE_1];
            key_2_down = keys[SDL_SCANCODE_2];
            key_3_down = keys[SDL_SCANCODE_3];
            key_t_down = keys[SDL_SCANCODE_T];
            key_m_down = keys[SDL_SCANCODE_M];
            key_b_down = keys[SDL_SCANCODE_B];
            key_p_down = keys[SDL_SCANCODE_P];
        }
#endif

        sim_time += dt;

        // Update active texture
        if (enable_texture) {
            mat.texture_data = textures[current_tex_idx].pixels.data();
            mat.texture_width = textures[current_tex_idx].width;
            mat.texture_height = textures[current_tex_idx].height;
        } else {
            mat.texture_data = nullptr;
        }

        // Animate model rotation with smooth dual-axis quaternion
        Quat rot_y =
            Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), static_cast<float>(sim_time) * 0.75f);
        Quat rot_x =
            Quat::from_axis_angle(Vec3(1.0f, 0.0f, 0.0f), static_cast<float>(sim_time) * 0.40f);
        Mat4 model = translate(Vec3(0.0f, 0.15f, 0.0f)) * (rot_y * rot_x).to_mat4();

        // Pedestal transformation
        Mat4 pedestal_model = translate(Vec3(0.0f, -1.25f, 0.0f)) * scale(Vec3(2.4f, 0.15f, 2.4f));

        // Orbit dynamic cyan fill light
        float orbit_radius = 2.1f;
        Vec3 light_pos(std::cos(static_cast<float>(sim_time) * 1.6f) * orbit_radius,
                       0.6f + std::sin(static_cast<float>(sim_time) * 2.2f) * 0.6f,
                       std::sin(static_cast<float>(sim_time) * 1.6f) * orbit_radius);
        lights[1].direction_or_position = light_pos;

        // Orbiting glowing light orb marker
        Mat4 orb_model = translate(light_pos) * scale(0.12f);

        // Clear with dark studio vignette background
        fb.clear(0xFF0F0F16);

        // 1. Render Pedestal Ground Plane
        draw_triangles(fb, mesh_cube, pedestal_model, camera, pedestal_mat, lights, config);

        // 2. Render Main 3D Model
        draw_triangles(fb, mesh_cube, model, camera, mat, lights, config);

        // 3. Render Glowing Light Orb
        draw_triangles(fb, mesh_cube, orb_model, camera, orb_mat, lights, config);

        // 4. Post-Processing Pass (Bloom, Vignette, Tone-Mapping)
        if (enable_post_process) {
            post_processor.apply(fb, post_cfg);
        }

        // 5. Overlay Crisp HUD
        char buf[128];
        std::snprintf(buf, sizeof(buf), "FPS: %.0f (%.2f ms) | 3D Studio Lighting | Post-FX: %s",
                      fps, frame_ms, enable_post_process ? "ON (Bloom+Vignette)" : "OFF");
        fb.draw_string(10, 10, buf, 0xFF00FFCC, 1);

        const char* shading_str = (config.shading == ShadingMode::Flat)      ? "Flat"
                                  : (config.shading == ShadingMode::Gouraud) ? "Gouraud"
                                                                             : "Blinn-Phong";
        const char* mat_names[] = {"Sci-Fi Metal Panel", "Glowing Crystal", "Carved Stone Wall",
                                   "Checkerboard"};
        std::snprintf(buf, sizeof(buf), "Shading: %s | Texture: %s [%s] | Culling: %s", shading_str,
                      enable_texture ? "ON" : "OFF", mat_names[current_tex_idx],
                      config.backface_culling ? "ON" : "OFF");
        fb.draw_string(10, 24, buf, 0xFFFFAA00, 1);

        fb.draw_string(
            10, 38, "Keys: [1/2/3] Shading | [T] Texture | [M] Material | [B] Cull | [P] Post-FX",
            0xFFAAAAAA, 1);

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
