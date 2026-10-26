#pragma once

#include <prism/render/rasterizer.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/camera.hpp>
#include <prism/math/geometry.hpp>
#include <prism/math/mat4.hpp>
#include <vector>
#include <future>

namespace prism::render {

// Reusable scratch memory for zero per-frame heap allocations
struct RenderScratchBuffer {
    std::vector<ClipSpaceVertex> clip_a;
    std::vector<ClipSpaceVertex> clip_b;

    RenderScratchBuffer() {
        clip_a.reserve(16);
        clip_b.reserve(16);
    }
};

// Check if an object bounding box is visible inside camera frustum
[[nodiscard]] inline bool is_visible(const math::Frustum& frustum, const math::AABB& bounds) noexcept {
    return frustum.intersects_aabb(bounds);
}

struct RenderMeshInstance {
    const std::vector<Triangle>* mesh{nullptr};
    math::Mat4 model_matrix;
};

// Batched multi-threaded rasterizer: transforms and rasterizes entire scene across worker threads
void draw_scene_multithreaded(Framebuffer& fb,
                              const std::vector<RenderMeshInstance>& instances,
                              const Camera& camera,
                              const Material& material,
                              const std::vector<Light>& lights,
                              const RenderConfig& config,
                              int num_threads = 4) noexcept;

} // namespace prism::render
