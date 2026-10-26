#include <prism/core/window.hpp>
#include <prism/game/dungeon_logic.hpp>
#include <prism/math/bvh.hpp>
#include <prism/math/constants.hpp>
#include <prism/math/intersection.hpp>
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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using namespace prism;
using namespace prism::math;
using namespace prism::render;
using namespace prism::game;

static constexpr int kWidth = 800;
static constexpr int kHeight = 600;

struct Entity {
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 extents{0.5f, 0.5f, 0.5f};
    Vec3 color{1.0f, 1.0f, 1.0f};
    bool active{true};
    bool is_crystal{false};
    int light_index{-1};

    [[nodiscard]] AABB bounds() const noexcept {
        return AABB(position - extents, position + extents);
    }
};

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

    // Player State
    Vec3 player_pos{0.0f, 0.5f, 0.0f};
    Vec3 prev_player_pos = player_pos;
    float player_yaw = 0.0f;
    float player_pitch = 0.0f;
    float player_radius = 0.35f;
    int score = 0;
    GameState game_state = GameState::Playing;

    Camera camera;
    camera.position = player_pos;
    camera.aspect = static_cast<float>(kWidth) / kHeight;
    camera.fov_y = deg_to_rad(65.0f);
    camera.near_z = 0.1f;
    camera.far_z = 50.0f;

    // Enemy State
    Enemy enemy;
    enemy.position = Vec3(4.0f, 0.5f, 4.0f);
    enemy.patrol_origin = enemy.position;
    enemy.patrol_target = Vec3(-4.0f, 0.5f, 4.0f);
    Vec3 prev_enemy_pos = enemy.position;

    auto cube_mesh = make_cube();

    // Procedural Textures
    auto wall_tex = make_stone_wall_texture(256);
    auto floor_tex = make_flagstone_floor_texture(256);
    auto gem_tex = make_glowing_crystal_texture(128);
    auto enemy_tex = make_scifi_metal_texture(128);

    Material wall_mat;
    wall_mat.ambient = Vec3(0.06f, 0.06f, 0.09f);
    wall_mat.diffuse = Vec3(0.8f, 0.8f, 0.8f);
    wall_mat.specular = Vec3(0.3f, 0.3f, 0.3f);
    wall_mat.shininess = 16.0f;
    wall_mat.texture_data = wall_tex.pixels.data();
    wall_mat.texture_width = wall_tex.width;
    wall_mat.texture_height = wall_tex.height;

    Material floor_mat = wall_mat;
    floor_mat.texture_data = floor_tex.pixels.data();
    floor_mat.texture_width = floor_tex.width;
    floor_mat.texture_height = floor_tex.height;

    Material gem_mat;
    gem_mat.ambient = Vec3(0.3f, 0.3f, 0.1f);
    gem_mat.diffuse = Vec3(0.9f, 0.7f, 0.2f);
    gem_mat.specular = Vec3(1.0f, 1.0f, 1.0f);
    gem_mat.shininess = 64.0f;
    gem_mat.texture_data = gem_tex.pixels.data();
    gem_mat.texture_width = gem_tex.width;
    gem_mat.texture_height = gem_tex.height;

    Material enemy_mat;
    enemy_mat.ambient = Vec3(0.4f, 0.1f, 0.1f);
    enemy_mat.diffuse = Vec3(0.9f, 0.2f, 0.2f);
    enemy_mat.specular = Vec3(1.0f, 0.5f, 0.5f);
    enemy_mat.shininess = 32.0f;
    enemy_mat.texture_data = enemy_tex.pixels.data();
    enemy_mat.texture_width = enemy_tex.width;
    enemy_mat.texture_height = enemy_tex.height;

    // Dungeon Map
    constexpr int kMapW = 9;
    constexpr int kMapH = 9;
    const int dungeon_map[kMapH][kMapW] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1}, {1, 0, 0, 0, 1, 0, 0, 2, 1}, {1, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 2, 1, 0, 0, 0, 1, 0, 1}, {1, 0, 1, 1, 0, 1, 1, 0, 1}, {1, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 0, 1, 1, 1, 0, 1}, {1, 2, 0, 0, 0, 0, 2, 0, 1}, {1, 1, 1, 1, 1, 1, 1, 1, 1},
    };

    std::vector<Entity> walls;
    std::vector<Entity> crystals;
    std::vector<Light> lights;

    Light moon;
    moon.type = LightType::Directional;
    moon.direction_or_position = normalize(Vec3(0.3f, -1.0f, 0.4f));
    moon.color = Vec3(0.4f, 0.5f, 0.7f);
    moon.intensity = 0.5f;
    lights.push_back(moon);

    // Enemy eye glow point light
    Light enemy_light;
    enemy_light.type = LightType::Point;
    enemy_light.direction_or_position = enemy.position;
    enemy_light.color = Vec3(1.0f, 0.1f, 0.1f);
    enemy_light.intensity = 3.0f;
    enemy_light.radius = 4.0f;
    lights.push_back(enemy_light);
    int enemy_light_idx = static_cast<int>(lights.size()) - 1;

    Vec3 gem_colors[] = {
        {1.0f, 0.3f, 0.2f},
        {0.2f, 0.9f, 0.4f},
        {0.2f, 0.5f, 1.0f},
        {1.0f, 0.8f, 0.1f},
    };
    int gem_color_idx = 0;

    std::vector<BVHPrimitive> wall_prims;
    for (int r = 0; r < kMapH; ++r) {
        for (int c = 0; c < kMapW; ++c) {
            float wx = (c - kMapW / 2) * 2.0f;
            float wz = (r - kMapH / 2) * 2.0f;

            if (dungeon_map[r][c] == 1) {
                Entity w;
                w.position = Vec3(wx, 1.0f, wz);
                w.extents = Vec3(1.0f, 1.0f, 1.0f);
                int wid = static_cast<int>(walls.size());
                walls.push_back(w);
                wall_prims.emplace_back(w.bounds(), wid);
            } else if (dungeon_map[r][c] == 2) {
                Entity g;
                g.position = Vec3(wx, 0.6f, wz);
                g.extents = Vec3(0.3f, 0.3f, 0.3f);
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

    // Build spatial BVH over dungeon walls
    BVH wall_bvh;
    wall_bvh.build(wall_prims);

    const int total_crystals = static_cast<int>(crystals.size());

    // Visual Effects & Post-Processing Setup
    PostProcessor post_processor;
    post_processor.init(kWidth, kHeight);
    PostProcessConfig post_cfg;
    post_cfg.enable_fog = true;
    post_cfg.fog_near = 2.0f;
    post_cfg.fog_far = 13.5f;
    post_cfg.fog_color = 0xFF0D0D14; // Atmospheric midnight dungeon mist
    post_cfg.enable_bloom = true;
    post_cfg.bloom_threshold = 0.58f;
    post_cfg.bloom_intensity = 0.65f;
    post_cfg.enable_vignette = true;
    post_cfg.vignette_radius = 0.70f;
    post_cfg.vignette_softness = 0.50f;
    post_cfg.enable_color_grade = true;
    post_cfg.contrast = 1.16f;
    post_cfg.saturation = 1.15f;
    post_cfg.exposure = 1.05f;

    struct DustMote {
        Vec3 pos;
        float seed;
    };
    std::vector<DustMote> particles;
    particles.reserve(80);
    for (int i = 0; i < 80; ++i) {
        float px = static_cast<float>((i * 13) % 16 - 8) * 0.9f;
        float py = 0.2f + static_cast<float>((i * 7) % 15) * 0.1f;
        float pz = static_cast<float>((i * 23) % 16 - 8) * 0.9f;
        particles.push_back({Vec3(px, py, pz), static_cast<float>(i)});
    }

    RenderConfig config;
    config.shading = ShadingMode::Phong;
    config.backface_culling = true;
    config.depth_test = true;

    core::Window window("Prism – Dungeon Crawler & AI Chase", kWidth, kHeight, headless);

    // Fixed-timestep accumulator loop (60 Hz physics/logic)
    constexpr double kFixedDeltaTime = 1.0 / 60.0;
    double accumulator = 0.0;
    auto t_prev = std::chrono::high_resolution_clock::now();
    auto t_fps_start = t_prev;
    int fps_frame_count = 0;
    double fps = 0.0;
    double frame_ms = 0.0;
    int frame_count = 0;
    double sim_time = 0.0;

    while (window.is_running()) {
        if (headless && frame_count >= headless_frames)
            break;

        auto t_now = std::chrono::high_resolution_clock::now();
        double dt = std::chrono::duration<double>(t_now - t_prev).count();
        if (headless)
            dt = kFixedDeltaTime; // Deterministic scripted playback advances one fixed tick per
                                  // rendered frame.
        if (dt > 0.25)
            dt = 0.25; // Clamp for stability
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

        // Input Gathering
        float move_forward = 0.0f;
        float move_strafe = 0.0f;
        float turn_yaw = 0.0f;
        float turn_pitch = 0.0f;
        bool interact = false;

#if !defined(PRISM_HEADLESS)
        const uint8_t* keys = SDL_GetKeyboardState(nullptr);
        if (keys && game_state == GameState::Playing) {
            if (keys[SDL_SCANCODE_W])
                move_forward += 1.0f;
            if (keys[SDL_SCANCODE_S])
                move_forward -= 1.0f;
            if (keys[SDL_SCANCODE_A])
                move_strafe -= 1.0f;
            if (keys[SDL_SCANCODE_D])
                move_strafe += 1.0f;
            if (keys[SDL_SCANCODE_LEFT])
                turn_yaw += 2.0f;
            if (keys[SDL_SCANCODE_RIGHT])
                turn_yaw -= 2.0f;
            if (keys[SDL_SCANCODE_UP])
                turn_pitch += 1.5f;
            if (keys[SDL_SCANCODE_DOWN])
                turn_pitch -= 1.5f;
            if (keys[SDL_SCANCODE_SPACE])
                interact = true;
        }
#endif
        if (headless) {
            // Scripted deterministic input
            if (frame_count < 20) {
                turn_yaw += 0.8f;
                move_forward += 1.0f;
            } else if (frame_count < 45) {
                move_forward += 1.0f;
                interact = true;
            } else {
                turn_yaw -= 0.5f;
                move_strafe += 1.0f;
            }
        }

        // Fixed-timestep logic updates
        accumulator += dt;
        while (accumulator >= kFixedDeltaTime) {
            prev_player_pos = player_pos;
            prev_enemy_pos = enemy.position;
            sim_time += kFixedDeltaTime;

            if (game_state == GameState::Playing) {
                player_yaw += turn_yaw * static_cast<float>(kFixedDeltaTime);
                player_pitch += turn_pitch * static_cast<float>(kFixedDeltaTime);
                player_pitch = std::clamp(player_pitch, -deg_to_rad(80.0f), deg_to_rad(80.0f));

                Quat q_yaw = Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), player_yaw);
                Quat q_pitch = Quat::from_axis_angle(Vec3(1.0f, 0.0f, 0.0f), player_pitch);
                camera.orientation = q_yaw * q_pitch;

                Vec3 forward = q_yaw.rotate(Vec3(0.0f, 0.0f, -1.0f));
                Vec3 right = q_yaw.rotate(Vec3(1.0f, 0.0f, 0.0f));
                float speed = 3.0f;
                Vec3 move_dir = forward * move_forward + right * move_strafe;
                if (length_squared(move_dir) > 0.01f) {
                    move_dir = normalize(move_dir);
                    player_pos =
                        player_pos + move_dir * (speed * static_cast<float>(kFixedDeltaTime));
                }

                // BVH-accelerated wall collision with slide resolution
                Sphere player_sphere(player_pos, player_radius);
                wall_bvh.query_sphere(player_sphere, [&](int wall_id) {
                    const auto& wb = walls[wall_id].bounds();
                    Vec3 closest(clamp(player_pos.x, wb.min.x, wb.max.x),
                                 clamp(player_pos.y, wb.min.y, wb.max.y),
                                 clamp(player_pos.z, wb.min.z, wb.max.z));
                    Vec3 diff = player_pos - closest;
                    float d = length(diff);
                    if (d > kEps) {
                        Vec3 n = diff * (1.0f / d);
                        player_pos = player_pos + n * (player_radius - d);
                    } else {
                        player_pos.x = wb.max.x + player_radius;
                    }
                });
                player_pos.y = 0.5f;

                // Update Enemy AI
                enemy.update(static_cast<float>(kFixedDeltaTime), player_pos, wall_bvh);
                lights[enemy_light_idx].direction_or_position = enemy.position;

                // Check lose condition (enemy catches player)
                if (length(player_pos - enemy.position) < (player_radius + enemy.radius)) {
                    game_state = GameState::Lost;
                }

                // Collectible raycast
                if (interact) {
                    Ray forward_ray(player_pos, camera.forward());
                    for (auto& g : crystals) {
                        if (!g.active)
                            continue;
                        Sphere gs(g.position, 0.5f);
                        float t1 = 0.0f, t2 = 0.0f;
                        if (intersect_ray_sphere(forward_ray, gs, t1, t2)) {
                            float t_hit = (t1 > 0.0f) ? t1 : t2;
                            if (t_hit > 0.0f && t_hit < 3.0f) {
                                g.active = false;
                                ++score;
                                if (g.light_index >= 0 &&
                                    g.light_index < static_cast<int>(lights.size())) {
                                    lights[g.light_index].intensity = 0.0f;
                                }
                            }
                        }
                    }
                    if (score >= total_crystals) {
                        game_state = GameState::Won;
                    }
                }
            }

            accumulator -= kFixedDeltaTime;
        }

        // Render Interpolation
        float alpha = static_cast<float>(accumulator / kFixedDeltaTime);
        Vec3 render_player_pos = lerp(prev_player_pos, player_pos, alpha);
        Vec3 render_enemy_pos = lerp(prev_enemy_pos, enemy.position, alpha);
        camera.position = render_player_pos;

        // Clear Framebuffer
        fb.clear(0xFF101018);

        // Render floor
        for (int r = 0; r < kMapH; ++r) {
            for (int c = 0; c < kMapW; ++c) {
                if (dungeon_map[r][c] != 1) {
                    float wx = (c - kMapW / 2) * 2.0f;
                    float wz = (r - kMapH / 2) * 2.0f;
                    Mat4 floor_mat4 =
                        translate(Vec3(wx, -0.05f, wz)) * scale(Vec3(2.0f, 0.1f, 2.0f));
                    draw_triangles(fb, cube_mesh, floor_mat4, camera, floor_mat, lights, config);
                }
            }
        }

        // Render walls
        for (const auto& w : walls) {
            Mat4 wm = translate(w.position) * scale(w.extents * 2.0f);
            draw_triangles(fb, cube_mesh, wm, camera, wall_mat, lights, config);
        }

        // Render crystals with dynamic floating bob & pulsating point lights
        for (auto& g : crystals) {
            if (!g.active)
                continue;
            float bob = std::sin(static_cast<float>(sim_time) * 3.0f + g.position.x) * 0.10f;
            Vec3 pos = g.position + Vec3(0.0f, bob, 0.0f);
            Quat rot_y =
                Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), static_cast<float>(sim_time) * 2.5f);
            Quat rot_z = Quat::from_axis_angle(Vec3(0.707f, 0.0f, 0.707f), 0.35f);
            Mat4 gm = translate(pos) * (rot_y * rot_z).to_mat4() * scale(g.extents * 2.0f);
            draw_triangles(fb, cube_mesh, gm, camera, gem_mat, lights, config);

            // Modulate attached light position and pulsating intensity
            if (g.light_index >= 0 && static_cast<size_t>(g.light_index) < lights.size()) {
                lights[g.light_index].direction_or_position = pos;
                lights[g.light_index].intensity =
                    2.2f + 0.6f * std::sin(static_cast<float>(sim_time) * 4.0f + g.position.z);
            }
        }

        // Render Enemy
        Quat enemy_rot =
            Quat::from_axis_angle(Vec3(0.0f, 1.0f, 0.0f), static_cast<float>(sim_time) * 1.5f);
        Mat4 em = translate(render_enemy_pos) * enemy_rot.to_mat4() *
                  scale(Vec3(enemy.radius * 2.0f, 0.8f, enemy.radius * 2.0f));
        draw_triangles(fb, cube_mesh, em, camera, enemy_mat, lights, config);

        // 3D Floating Dust / Ember Particles
        Mat4 vp = camera.proj_matrix() * camera.view_matrix();
        for (auto& p : particles) {
            p.pos.y += static_cast<float>(dt) * 0.25f;
            if (p.pos.y > 1.8f)
                p.pos.y = 0.15f;
            p.pos.x += std::sin(p.pos.y * 3.0f + p.seed) * 0.003f;
            p.pos.z += std::cos(p.pos.y * 3.0f + p.seed) * 0.003f;

            Vec4 cp = vp * Vec4(p.pos, 1.0f);
            if (cp.w > 0.2f && cp.z > 0.0f && cp.z < cp.w) {
                float inv_w = 1.0f / cp.w;
                float ndc_x = cp.x * inv_w;
                float ndc_y = cp.y * inv_w;
                float ndc_z = cp.z * inv_w;
                if (ndc_x >= -1.0f && ndc_x <= 1.0f && ndc_y >= -1.0f && ndc_y <= 1.0f) {
                    int sx = static_cast<int>((ndc_x * 0.5f + 0.5f) * kWidth);
                    int sy = static_cast<int>((1.0f - (ndc_y * 0.5f + 0.5f)) * kHeight);
                    if (sx >= 0 && sx < kWidth - 1 && sy >= 0 && sy < kHeight - 1) {
                        float depth = ndc_z;
                        if (depth < fb.get_depth(sx, sy)) {
                            fb.set_pixel(sx, sy, 0xFFFFDD66);
                            fb.set_pixel(sx + 1, sy, 0x88FFAA33);
                            fb.set_pixel(sx, sy + 1, 0x88FFAA33);
                        }
                    }
                }
            }
        }

        // Apply Atmospheric Fog, Glowing Bloom, Vignette, and Color Grading!
        post_processor.apply(fb, post_cfg);

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
        // Mini-map gems
        for (const auto& g : crystals) {
            if (!g.active)
                continue;
            int gc = static_cast<int>((g.position.x / 2.0f) + kMapW / 2);
            int gr = static_cast<int>((g.position.z / 2.0f) + kMapH / 2);
            fb.set_pixel(mm_x + gc * cell_size + cell_size / 2,
                         mm_y + gr * cell_size + cell_size / 2, 0xFFFFDD00);
        }
        // Mini-map enemy (Red)
        int ec = static_cast<int>((render_enemy_pos.x / 2.0f) + kMapW / 2);
        int er = static_cast<int>((render_enemy_pos.z / 2.0f) + kMapH / 2);
        fb.set_pixel(mm_x + ec * cell_size + cell_size / 2, mm_y + er * cell_size + cell_size / 2,
                     0xFFFF2222);

        // Mini-map player (Green)
        int pc = static_cast<int>((render_player_pos.x / 2.0f) + kMapW / 2);
        int pr = static_cast<int>((render_player_pos.z / 2.0f) + kMapH / 2);
        fb.set_pixel(mm_x + pc * cell_size + cell_size / 2, mm_y + pr * cell_size + cell_size / 2,
                     0xFF00FF88);

        // Crosshair
        int cx = kWidth / 2;
        int cy = kHeight / 2;
        fb.draw_line(cx - 6, cy, cx + 6, cy, 0x80FFFFFF);
        fb.draw_line(cx, cy - 6, cx, cy + 6, 0x80FFFFFF);

        // HUD overlay
        char buf[128];
        std::snprintf(buf, sizeof(buf), "Crystals: %d / %d", score, total_crystals);
        fb.draw_string(10, 10, buf, 0xFFFFCC00, 2);

        std::snprintf(buf, sizeof(buf), "Enemy: %s | FPS: %.0f (%.2f ms)",
                      (enemy.state == EnemyState::Chase) ? "CHASING!" : "Patrolling", fps,
                      frame_ms);
        fb.draw_string(10, 32, buf, (enemy.state == EnemyState::Chase) ? 0xFFFF3333 : 0xFF00FFCC,
                       1);

        fb.draw_string(10, 48, "WASD: Walk | Arrows/Mouse: Look | SPACE: Collect Crystal",
                       0xFFAAAAAA, 1);

        if (game_state == GameState::Won) {
            fb.draw_string(kWidth / 2 - 100, kHeight / 2 - 20, "YOU WON! ALL CRYSTALS COLLECTED!",
                           0xFF00FF88, 2);
        } else if (game_state == GameState::Lost) {
            fb.draw_string(kWidth / 2 - 80, kHeight / 2 - 20, "GAME OVER! CAUGHT BY ENEMY!",
                           0xFFFF2222, 2);
        }

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
