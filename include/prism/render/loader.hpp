#pragma once

#include <functional>
#include <prism/math/mat4.hpp>
#include <prism/math/vec3.hpp>
#include <prism/render/framebuffer.hpp>
#include <prism/render/rasterizer.hpp>
#include <string>
#include <vector>

namespace prism::render {

// ── Simple OBJ loader ─────────────────────────────────────────────────────────
// Supports: v, vt, vn, f (with triangulation of quads and polygons)
// Returns empty vector on parse failure.
[[nodiscard]] std::vector<Triangle> load_obj(const std::string& filepath);

// ── Texture loader ─────────────────────────────────────────────────────────────
// Returns width, height, and RGBA data allocated with new[].
// Caller must delete[]. Returns nullptr on failure.
// Supports PPM (P6 binary) and trivially extensible.
struct TextureData {
    std::vector<uint32_t> pixels;
    int width{0};
    int height{0};
    [[nodiscard]] bool valid() const noexcept { return !pixels.empty(); }
};
[[nodiscard]] TextureData load_ppm(const std::string& filepath);

// ── Procedural textures ────────────────────────────────────────────────────────
[[nodiscard]] TextureData make_checkerboard(int size, uint32_t c0 = 0xFFFFFFFF,
                                            uint32_t c1 = 0xFF333333);
[[nodiscard]] TextureData make_stone_wall_texture(int size);
[[nodiscard]] TextureData make_flagstone_floor_texture(int size);
[[nodiscard]] TextureData make_glowing_crystal_texture(int size);
[[nodiscard]] TextureData make_scifi_metal_texture(int size);

} // namespace prism::render
