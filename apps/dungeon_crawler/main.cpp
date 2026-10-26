#include <prism/core/window.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/rasterizer.hpp>
#include <prism/render/camera.hpp>
#include <prism/render/loader.hpp>
#include <prism/math/quaternion.hpp>
#include <prism/math/transform.hpp>
#include <prism/math/constants.hpp>
#include <prism/math/intersection.hpp>

#if !defined(PRISM_HEADLESS)
#include <SDL2/SDL.h>
#endif

#include <cstdio>
#include <string>
#include <vector>
#include <chrono>
#include <cstring>
#include <cmath>
#include <algorithm>

using namespace prism;
using namespace prism::math;
using namespace prism::render;

static constexpr int kWidth  = 800;
static constexpr int kHeight = 600;

// Game entity
struct Entity {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 extents{0.5f, 0.5f, 0.5f};
    Vec3 color{1.0f, 1.0f, 1.0f};
    bool active{true};
    bool is_crystal{false};
    int  light_index{-1};

    [[nodiscard]] AABB bounds() const noexcept {
        return AABB(position - extents, position + extents);
    }
};

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

    // Player & Camera
    Camera camera;
    camera.position = Vec3(0.0f, 0.5f, 0.0f);
    camera.aspect   = static_cast<float>(kWidth) / kHeight;
    camera.fov_y    = deg_to_rad(65.0f);
    camera.near_z   = 0.1f;
    camera.far_z    = 50.0f;

    float player_yaw   = 0.0f; // radians
    float player_pitch = 0.0f;
    float player_radius = 0.35f;
    int   score = 0;

    // Load unit cube mesh
    auto cube_mesh = make_cube();

    // Procedural textures
    auto wall_tex  = make_checkerboard(128, 0xFF888899, 0xFF444455);
    auto floor_tex = make_checkerboard(128, 0xFF555566, 0xFF222233);
    auto gem_tex   = make_checkerboard(64,  0xFFFFDD44, 0xFFFF8800);

    Material wall_mat;
    wall_mat.ambient  = Vec3(0.05f, 0.05f, 0.08f);
    wall_mat.diffuse  = Vec3(0.8f, 0.8f, 0.8f);
    wall_mat.specular = Vec3(0.3f, 0.3f, 0.3f);
    wall_mat.shininess = 16.0f;
    wall_mat.texture_data   = wall_tex.pixels.data();
    wall_mat.texture_width  = wall_tex.width;
    wall_mat.texture_height = wall_tex.height;

    Material floor_mat = wall_mat;
    floor_mat.texture_data   = floor_tex.pixels.data();
    floor_mat.texture_width  = floor_tex.width;
    floor_mat.texture_height = floor_tex.height;

    Material gem_mat;
    gem_mat.ambient  = Vec3(0.3f, 0.3f, 0.1f);
    gem_mat.diffuse  = Vec3(0.9f, 0.7f, 0.2f);
    gem_mat.specular = Vec3(1.0f, 1.0f, 1.0f);
    gem_mat.shininess = 64.0f;
    gem_mat.texture_data   = gem_tex.pixels.data();
    gem_mat.texture_width  = gem_tex.width;
    gem_mat.texture_height = gem_tex.height;

    // Build Dungeon map:
    // 0 = empty, 1 = wall, 2 = gem crystal
    constexpr int kMapW = 9;
    constexpr int kMapH = 9;
    const int dungeon_map[kMapH][kMapW] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 1, 0, 0, 2, 1},
        {1, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 2, 1, 0, 0, 0, 1, 0, 1},
        {1, 0, 1, 1, 0, 1, 1, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 1, 1, 1, 0, 1},
        {1, 2, 0, 0, 0, 0, 2, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

    std::vector<Entity> walls;
    std::vector<Entity> crystals;
    std::vector<Light>  lights;

    // Global ambient directional moonlight
    Light moon;
    moon.type = LightType::Directional;
    moon.direction_or_position = normalize(Vec3(0.3f, -1.0f, 0.4f));
    moon.color = Vec3(0.4f, 0.5f, 0.7f);
    moon.intensity = 0.5f;
    lights.push_back(moon);

    // Crystal colors for dynamic point lights
    Vec3 gem_colors[] = {
        {1.0f, 0.3f, 0.2f}, // Ruby red
        {0.2f, 0.9f, 0.4f}, // Emerald green
        {0.2f, 0.5f, 1.0f}, // Sapphire blue
        {1.0f, 0.8f, 0.1f}, // Topaz yellow
    };
    int gem_color_idx = 0;

    for (int r = 0; r < kMapH; ++r) {
        for (int c = 0; c < kMapW; ++c) {
            float wx = (c - kMapW / 2) * 2.0f;
            float wz = (r - kMapH / 2) * 2.0f;

            if (dungeon_map[r][c] == 1) {
                Entity w;
                w.position = Vec3(wx, 1.0f, wz);
                w.extents  = Vec3(1.0f, 1.0f, 1.0f);
                walls.push_back(w);
            } else if (dungeon_map[r][c] == 2) {
                Entity g;
                g.position = Vec3(wx, 0.6f, wz);
                g.extents  = Vec3(0.3f, 0.3f, 0.3f);
                g.is_crystal = true;
                g.color = gem_colors[gem_color_idx % 4];

                Light pl;
                pl.type = LightType::Point;
                pl.direction_or_position = g.position;
                pl.color = g.color;
                pl.intensity = 2.5f;
                pl.radius = 4.5f;

                g.light_index = static_cast<int>(lights.size());
                lights.push_back(pl);
                crystals.push_back(g);
                ++gem_color_idx;
            }
        }
    }

    const int total_crystals = static_cast<int>(crystals.size());

    RenderConfig config;
    config.shading = ShadingMode::Phong;
    config.backface_culling = true;
    config.depth_test = true;
    config.wireframe = false;

    core::Window window("Prism – Stage 4 Dungeon Crawler", kWidth, kHeight, headless);

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
        if (dt > 0.1) dt = 0.1;
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

        // Player controls
        float move_forward = 0.0f;
        float move_strafe  = 0.0f;
        float turn_yaw     = 0.0f;
        float turn_pitch   = 0.0f;
        bool  interact     = false;

#if !defined(PRISM_HEADLESS)
        const uint8_t* keys = SDL_GetKeyboardState(nullptr);
        if (keys) {
            if (keys[SDL_SCANCODE_W]) move_forward += 1.0f;
            if (keys[SDL_SCANCODE_S]) move_forward -= 1.0f;
            if (keys[SDL_SCANCODE_A]) move_strafe  -= 1.0f;
            if (keys[SDL_SCANCODE_D]) move_strafe  += 1.0f;
            if (keys[SDL_SCANCODE_LEFT])  turn_yaw   += 1.5f;
            if (keys[SDL_SCANCODE_RIGHT]) turn_yaw   -= 1.5f;
            if (keys[SDL_SCANCODE_UP])    turn_pitch += 1.0f;
            if (keys[SDL_SCANCODE_DOWN])  turn_pitch -= 1.0f;
            if (keys[SDL_SCANCODE_SPACE]) interact   = true;
        }
#endif
        if (headless) {
            turn_yaw += 0.5f;
            move_forward += 0.2f;
            if (frame_count == 2) interact = true;
        }

        player_yaw   += turn_yaw   * static_cast<float>(dt);
        player_pitch += turn_pitch * static_cast<float>(dt);
        player_pitch = std::clamp(player_pitch, -deg_to_rad(80.0f), deg_to_rad(80.0f));

        Quat q_yaw   = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), player_yaw);
        Quat q_pitch = Quat::from_axis_angle(Vec3(1.0f, 0.0f, 0.0f), player_pitch);
        camera.orientation = q_yaw * q_pitch;

        Vec3 forward = q_yaw.rotate(Vec3(0.0f, 0.0f, -1.0f));
        Vec3 right   = q_yaw.rotate(Vec3(1.0f, 0.0f, 0.0f));
        float speed  = 2.5f;
        Vec3 move_dir = forward * move_forward + right * move_strafe;
        if (length_squared(move_dir) > 0.01f) {
            move_dir = normalize(move_dir);
            camera.position = camera.position + move_dir * (speed * static_cast<float>(dt));
        }

        // Collision response against walls (Sphere vs AABB)
        for (const auto& w : walls) {
            AABB wb = w.bounds();
            Sphere player_sphere(camera.position, player_radius);
            if (intersect_sphere_aabb(player_sphere, wb)) {
                Vec3 closest(clamp(camera.position.x, wb.min.x, wb.max.x),
                             clamp(camera.position.y, wb.min.y, wb.max.y),
                             clamp(camera.position.z, wb.min.z, wb.max.z));
                Vec3 diff = camera.position - closest;
                float d = length(diff);
                if (d > kEps) {
                    Vec3 n = diff * (1.0f / d);
                    camera.position = camera.position + n * (player_radius - d);
                } else {
                    camera.position.x = wb.max.x + player_radius;
                }
            }
        }
        camera.position.y = 0.5f;

        // Interact raycasting
        if (interact) {
            Ray forward_ray(camera.position, camera.forward());
            for (auto& g : crystals) {
                if (!g.active) continue;
                Sphere gs(g.position, 0.45f);
                float t1 = 0.0f, t2 = 0.0f;
                if (intersect_ray_sphere(forward_ray, gs, t1, t2)) {
                    float t_hit = (t1 > 0.0f) ? t1 : t2;
                    if (t_hit > 0.0f && t_hit < 3.0f) {
                        g.active = false;
                        ++score;
                        if (g.light_index >= 0 && g.light_index < static_cast<int>(lights.size())) {
                            lights[g.light_index].intensity = 0.0f;
                        }
                    }
                }
            }
        }

        // Animate crystals
        for (auto& g : crystals) {
            if (!g.active) continue;
            g.position.y = 0.6f + std::sin(static_cast<float>(sim_time) * 3.0f) * 0.1f;
            if (g.light_index >= 0) {
                lights[g.light_index].direction_or_position = g.position;
            }
        }

        // Clear framebuffer
        fb.clear(0xFF101018);

        // Render floor
        for (int r = 0; r < kMapH; ++r) {
            for (int c = 0; c < kMapW; ++c) {
                if (dungeon_map[r][c] != 1) {
                    float wx = (c - kMapW / 2) * 2.0f;
                    float wz = (r - kMapH / 2) * 2.0f;
                    Mat4 floor_mat4 = translate(Vec3(wx, -0.05f, wz)) * scale(Vec3(2.0f, 0.1f, 2.0f));
                    draw_triangles(fb, cube_mesh, floor_mat4, camera, floor_mat, lights, config);
                }
            }
        }

        // Render walls
        for (const auto& w : walls) {
            Mat4 wm = translate(w.position) * scale(w.extents * 2.0f);
            draw_triangles(fb, cube_mesh, wm, camera, wall_mat, lights, config);
        }

        // Render crystals
        for (const auto& g : crystals) {
            if (!g.active) continue;
            Quat rot = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), static_cast<float>(sim_time) * 2.5f);
            Mat4 gm = translate(g.position) * rot.to_mat4() * scale(g.extents * 2.0f);
            draw_triangles(fb, cube_mesh, gm, camera, gem_mat, lights, config);
        }

        // Mini-map in top-right corner
        int mm_x = kWidth - 110;
        int mm_y = 10;
        int mm_size = 100;
        int cell_size = mm_size / kMapW;

        for (int r = 0; r < kMapH; ++r) {
            for (int c = 0; c < kMapW; ++c) {
                int px0 = mm_x + c * cell_size;
                int py0 = mm_y + r * cell_size;
                uint32_t col = (dungeon_map[r][c] == 1) ? 0xFF555566 : 0xFF22222A;
                for (int dy = 0; dy < cell_size - 1; ++dy) {
                    for (int dx = 0; dx < cell_size - 1; ++dx) {
                        fb.set_pixel(px0 + dx, py0 + dy, col);
                    }
                }
            }
        }
        for (const auto& g : crystals) {
            if (!g.active) continue;
            int gc = static_cast<int>((g.position.x / 2.0f) + kMapW / 2);
            int gr = static_cast<int>((g.position.z / 2.0f) + kMapH / 2);
            fb.set_pixel(mm_x + gc * cell_size + cell_size / 2,
                         mm_y + gr * cell_size + cell_size / 2, 0xFFFFDD00);
        }
        int pc = static_cast<int>((camera.position.x / 2.0f) + kMapW / 2);
        int pr = static_cast<int>((camera.position.z / 2.0f) + kMapH / 2);
        int p_screen_x = mm_x + pc * cell_size + cell_size / 2;
        int p_screen_y = mm_y + pr * cell_size + cell_size / 2;
        fb.set_pixel(p_screen_x, p_screen_y, 0xFF00FF88);
        fb.set_pixel(p_screen_x + 1, p_screen_y, 0xFF00FF88);
        fb.set_pixel(p_screen_x, p_screen_y + 1, 0xFF00FF88);

        // Crosshair
        int cx = kWidth / 2;
        int cy = kHeight / 2;
        fb.draw_line(cx - 6, cy, cx + 6, cy, 0x80FFFFFF);
        fb.draw_line(cx, cy - 6, cx, cy + 6, 0x80FFFFFF);

        // HUD overlay
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Crystals: %d / %d", score, total_crystals);
        fb.draw_string(10, 10, buf, 0xFFFFCC00, 2);

        std::snprintf(buf, sizeof(buf), "FPS: %.0f  (%.2f ms) | Pos: (%.1f, %.1f)",
                      fps, frame_ms, camera.position.x, camera.position.z);
        fb.draw_string(10, 32, buf, 0xFF00FFCC, 1);

        fb.draw_string(10, 48, "WASD: Walk | Arrows/Mouse: Look | SPACE: Collect Crystal", 0xFFAAAAAA, 1);

        if (headless) {
            char path[64];
            std::snprintf(path, sizeof(path), "dungeon_frame_%04d.ppm", frame_count);
            (void)fb.save_ppm(path);
        } else {
            window.present(fb);
        }

        ++frame_count;
    }

    return 0;
}
