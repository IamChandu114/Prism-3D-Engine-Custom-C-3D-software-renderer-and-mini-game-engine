#pragma once

#include <algorithm>
#include <prism/core/thread_pool.hpp>
#include <prism/render/camera.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/rasterizer.hpp>
#include <vector>

namespace prism::render {

struct TileBin {
    int x_min{0};
    int x_max{0};
    int y_min{0};
    int y_max{0};
    std::vector<int> tri_indices;

    void clear() noexcept {
        tri_indices.clear(); // Keeps capacity!
    }
};

class TileRasterizer {
public:
    static constexpr int kTileSize = 64;

    TileRasterizer() = default;

    void init(int width, int height) {
        width_ = width;
        height_ = height;
        tiles_x_ = (width + kTileSize - 1) / kTileSize;
        tiles_y_ = (height + kTileSize - 1) / kTileSize;
        int total_tiles = tiles_x_ * tiles_y_;

        tiles_.resize(total_tiles);
        for (int ty = 0; ty < tiles_y_; ++ty) {
            for (int tx = 0; tx < tiles_x_; ++tx) {
                int idx = ty * tiles_x_ + tx;
                tiles_[idx].x_min = tx * kTileSize;
                tiles_[idx].x_max = std::min((tx + 1) * kTileSize - 1, width - 1);
                tiles_[idx].y_min = ty * kTileSize;
                tiles_[idx].y_max = std::min((ty + 1) * kTileSize - 1, height - 1);
                tiles_[idx].tri_indices.reserve(512); // Pre-allocate!
            }
        }
        cached_screen_tris_.reserve(65536);
    }

    // Render an array of triangles using tile binning and persistent thread pool
    void render_tiled(Framebuffer& fb, const std::vector<Triangle>& triangles,
                      const math::Mat4& model, const Camera& camera, const Material& material,
                      const std::vector<Light>& lights, const RenderConfig& config,
                      core::ThreadPool& pool);

    // Batched scene rendering
    struct MeshBatch {
        const std::vector<Triangle>* mesh{nullptr};
        math::Mat4 model;
    };

    void render_scene_tiled(Framebuffer& fb, const std::vector<MeshBatch>& batches,
                            const Camera& camera, const Material& material,
                            const std::vector<Light>& lights, const RenderConfig& config,
                            core::ThreadPool& pool);

    [[nodiscard]] int tile_count() const noexcept { return static_cast<int>(tiles_.size()); }

private:
    struct ScreenTri {
        ScreenSpaceVertex v[3];
        int tx_min, tx_max;
        int ty_min, ty_max;
    };

    int width_{0};
    int height_{0};
    int tiles_x_{0};
    int tiles_y_{0};
    std::vector<TileBin> tiles_;
    std::vector<ScreenTri> cached_screen_tris_;
};

} // namespace prism::render
