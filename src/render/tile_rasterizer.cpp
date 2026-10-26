#include <cmath>
#include <prism/math/constants.hpp>
#include <prism/render/pipeline.hpp>
#include <prism/render/tile_rasterizer.hpp>

namespace prism::render {

using namespace math;

static inline void rasterize_triangle_tile(Framebuffer& fb, const ScreenSpaceVertex& v0,
                                           const ScreenSpaceVertex& v1, const ScreenSpaceVertex& v2,
                                           const Material& material,
                                           const std::vector<Light>& lights, const Vec3& camera_pos,
                                           int x_clamp_min, int x_clamp_max, int y_clamp_min,
                                           int y_clamp_max) noexcept {
    int x_min = std::max(x_clamp_min, static_cast<int>(std::floor(std::min({v0.x, v1.x, v2.x}))));
    int x_max = std::min(x_clamp_max, static_cast<int>(std::ceil(std::max({v0.x, v1.x, v2.x}))));
    int y_min = std::max(y_clamp_min, static_cast<int>(std::floor(std::min({v0.y, v1.y, v2.y}))));
    int y_max = std::min(y_clamp_max, static_cast<int>(std::ceil(std::max({v0.y, v1.y, v2.y}))));

    if (x_min > x_max || y_min > y_max)
        return;

    float area = (v1.x - v0.x) * (v2.y - v0.y) - (v1.y - v0.y) * (v2.x - v0.x);
    if (area <= 0.0f)
        return;
    float inv_area = 1.0f / area;

    for (int y = y_min; y <= y_max; ++y) {
        for (int x = x_min; x <= x_max; ++x) {
            float px = static_cast<float>(x) + 0.5f;
            float py = static_cast<float>(y) + 0.5f;

            float w0 = ((v2.x - v1.x) * (py - v1.y) - (v2.y - v1.y) * (px - v1.x)) * inv_area;
            float w1 = ((v0.x - v2.x) * (py - v2.y) - (v0.y - v2.y) * (px - v2.x)) * inv_area;
            float w2 = ((v1.x - v0.x) * (py - v0.y) - (v1.y - v0.y) * (px - v0.x)) * inv_area;

            if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                continue;

            float depth = w0 * v0.z + w1 * v1.z + w2 * v2.z;
            if (depth >= fb.get_depth(x, y))
                continue;

            float iw0 = w0 * v0.inv_w;
            float iw1 = w1 * v1.inv_w;
            float iw2 = w2 * v2.inv_w;
            float iw_sum = iw0 + iw1 + iw2;
            if (iw_sum < kEps)
                continue;
            float inv_iw = 1.0f / iw_sum;
            float c0 = iw0 * inv_iw;
            float c1 = iw1 * inv_iw;
            float c2 = iw2 * inv_iw;

            Vec3 n = normalize(v0.normal * c0 + v1.normal * c1 + v2.normal * c2);
            Vec3 frag_pos(static_cast<float>(x), static_cast<float>(y), depth);
            Vec3 view_dir = normalize(camera_pos - frag_pos);

            Vec3 lit = material.ambient;
            for (const Light& light : lights) {
                Vec3 l_dir = (light.type == LightType::Directional)
                                 ? normalize(-light.direction_or_position)
                                 : normalize(light.direction_or_position - frag_pos);
                float ndotl = std::max(dot(n, l_dir), 0.0f);
                Vec3 h = normalize(l_dir + view_dir);
                float ndoth = std::max(dot(n, h), 0.0f);
                lit = lit + (material.diffuse * ndotl +
                             material.specular * std::pow(ndoth, material.shininess)) *
                                light.color * light.intensity;
            }

            uint8_t r = static_cast<uint8_t>(clamp(lit.x, 0.0f, 1.0f) * 255.0f);
            uint8_t g = static_cast<uint8_t>(clamp(lit.y, 0.0f, 1.0f) * 255.0f);
            uint8_t b = static_cast<uint8_t>(clamp(lit.z, 0.0f, 1.0f) * 255.0f);
            uint32_t col = (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) |
                           (static_cast<uint32_t>(g) << 8) | b;

            fb.set_pixel(x, y, col);
            fb.depth_data()[static_cast<size_t>(y) * fb.width() + x] = depth;
        }
    }
}

void TileRasterizer::render_scene_tiled(Framebuffer& fb, const std::vector<MeshBatch>& batches,
                                        const Camera& camera, const Material& material,
                                        const std::vector<Light>& lights,
                                        const RenderConfig& config, core::ThreadPool& pool) {
    if (tiles_.empty()) {
        init(fb.width(), fb.height());
    }

    // Clear tile bins without deallocating vectors
    for (auto& tile : tiles_) {
        tile.clear();
    }
    cached_screen_tris_.clear();

    Mat4 view = camera.view_matrix();
    Mat4 proj = camera.proj_matrix();
    int W = fb.width();
    int H = fb.height();

    // 1. Transform and bin triangles into tiles
    for (const auto& batch : batches) {
        if (!batch.mesh)
            continue;
        for (const Triangle& tri : *batch.mesh) {
            ClipSpaceVertex cv[3];
            for (int k = 0; k < 3; ++k) {
                Vec4 wpos = batch.model * Vec4(tri.v[k].position, 1.0f);
                cv[k].position = proj * (view * wpos);
                cv[k].normal = normalize((batch.model * Vec4(tri.v[k].normal, 0.0f)).xyz());
                cv[k].uv = tri.v[k].uv;
            }

            // Backface culling
            if (config.backface_culling) {
                Vec3 a = cv[0].position.xyz();
                Vec3 b = cv[1].position.xyz();
                Vec3 c = cv[2].position.xyz();
                if (cross(b - a, c - a).z <= 0.0f)
                    continue;
            }

            // Simple near clip test
            if (cv[0].position.w <= 0.0f || cv[1].position.w <= 0.0f || cv[2].position.w <= 0.0f)
                continue;

            ScreenTri st;
            for (int k = 0; k < 3; ++k) {
                NDCSpaceVertex nv = transform_clip_to_ndc(cv[k]);
                st.v[k] = transform_ndc_to_screen(nv, W, H);
            }

            // Determine overlapping tiles
            float min_x = std::min({st.v[0].x, st.v[1].x, st.v[2].x});
            float max_x = std::max({st.v[0].x, st.v[1].x, st.v[2].x});
            float min_y = std::min({st.v[0].y, st.v[1].y, st.v[2].y});
            float max_y = std::max({st.v[0].y, st.v[1].y, st.v[2].y});

            st.tx_min = std::clamp(static_cast<int>(min_x) / kTileSize, 0, tiles_x_ - 1);
            st.tx_max = std::clamp(static_cast<int>(max_x) / kTileSize, 0, tiles_x_ - 1);
            st.ty_min = std::clamp(static_cast<int>(min_y) / kTileSize, 0, tiles_y_ - 1);
            st.ty_max = std::clamp(static_cast<int>(max_y) / kTileSize, 0, tiles_y_ - 1);

            int tri_idx = static_cast<int>(cached_screen_tris_.size());
            cached_screen_tris_.push_back(st);

            // Bin into overlapping tiles
            for (int ty = st.ty_min; ty <= st.ty_max; ++ty) {
                for (int tx = st.tx_min; tx <= st.tx_max; ++tx) {
                    tiles_[ty * tiles_x_ + tx].tri_indices.push_back(tri_idx);
                }
            }
        }
    }

    // 2. Parallel rasterization pass over all tiles
    pool.parallel_for(
        static_cast<int>(tiles_.size()),
        [this, &fb, &material, &lights, &camera](int tile_idx) {
            const auto& tile = tiles_[tile_idx];
            if (tile.tri_indices.empty())
                return;

            for (int tri_idx : tile.tri_indices) {
                const auto& st = cached_screen_tris_[tri_idx];
                rasterize_triangle_tile(fb, st.v[0], st.v[1], st.v[2], material, lights,
                                        camera.position, tile.x_min, tile.x_max, tile.y_min,
                                        tile.y_max);
            }
        },
        4);
}

void TileRasterizer::render_tiled(Framebuffer& fb, const std::vector<Triangle>& triangles,
                                  const Mat4& model, const Camera& camera, const Material& material,
                                  const std::vector<Light>& lights, const RenderConfig& config,
                                  core::ThreadPool& pool) {
    MeshBatch batch{&triangles, model};
    render_scene_tiled(fb, {batch}, camera, material, lights, config, pool);
}

} // namespace prism::render
