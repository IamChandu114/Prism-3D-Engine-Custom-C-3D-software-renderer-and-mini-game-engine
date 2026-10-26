#pragma once

#include <cstdint>
#include <prism/render/framebuffer.hpp>
#include <vector>

namespace prism::render {

struct PostProcessConfig {
    // 1. Distance Depth Fog
    bool enable_fog{true};
    float fog_near{2.0f};
    float fog_far{16.0f};
    float camera_near{0.1f};
    float camera_far{50.0f};
    uint32_t fog_color{0xFF0E0E14}; // Deep midnight blue/indigo atmospheric haze

    // 2. High-Pass Bloom / Glow
    bool enable_bloom{true};
    float bloom_threshold{0.65f}; // Luminance threshold for glow
    float bloom_intensity{0.55f}; // Intensity of added bloom glow
    int blur_iterations{2};

    // 3. Cinematic Vignette
    bool enable_vignette{true};
    float vignette_radius{0.75f};
    float vignette_softness{0.45f};

    // 4. Color Grading & Contrast
    bool enable_color_grade{true};
    float contrast{1.15f};
    float saturation{1.10f};
    float exposure{1.05f};
};

class PostProcessor {
public:
    PostProcessor() = default;

    // Pre-allocate downscaled buffers to guarantee zero per-frame heap allocations
    void init(int width, int height);

    // Apply the full post-processing pipeline in-place
    void apply(Framebuffer& fb, const PostProcessConfig& config);

private:
    void extract_bright_pixels(const Framebuffer& fb, float threshold);
    void blur_bloom_buffer();
    void composite_effects(Framebuffer& fb, const PostProcessConfig& config);

    int width_{0};
    int height_{0};
    int bloom_w_{0};
    int bloom_h_{0};

    // Pre-allocated half-resolution buffers for bloom
    std::vector<uint32_t> bloom_buf0_;
    std::vector<uint32_t> bloom_buf1_;
};

} // namespace prism::render
