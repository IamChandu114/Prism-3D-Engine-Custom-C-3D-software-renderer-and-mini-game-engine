#include <algorithm>
#include <cmath>
#include <cstdint>
#include <prism/render/postprocess.hpp>

namespace prism::render {

static inline float smoothstep(float edge0, float edge1, float x) noexcept {
    float t = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

void PostProcessor::init(int width, int height) {
    width_ = width;
    height_ = height;
    bloom_w_ = std::max(1, width / 2);
    bloom_h_ = std::max(1, height / 2);

    size_t sz = static_cast<size_t>(bloom_w_) * bloom_h_;
    bloom_buf0_.assign(sz, 0);
    bloom_buf1_.assign(sz, 0);
}

void PostProcessor::extract_bright_pixels(const Framebuffer& fb, float threshold) {
    const uint32_t* src = fb.color_data();
    int src_w = fb.width();
    int src_h = fb.height();

    float thresh_luma = threshold * 255.0f;

    // Downsample 2x with box filter and threshold
    for (int by = 0; by < bloom_h_; ++by) {
        int sy0 = by * 2;
        int sy1 = std::min(sy0 + 1, src_h - 1);

        for (int bx = 0; bx < bloom_w_; ++bx) {
            int sx0 = bx * 2;
            int sx1 = std::min(sx0 + 1, src_w - 1);

            uint32_t p00 = src[static_cast<size_t>(sy0) * src_w + sx0];
            uint32_t p10 = src[static_cast<size_t>(sy0) * src_w + sx1];
            uint32_t p01 = src[static_cast<size_t>(sy1) * src_w + sx0];
            uint32_t p11 = src[static_cast<size_t>(sy1) * src_w + sx1];

            int r = (((p00 >> 16) & 0xFF) + ((p10 >> 16) & 0xFF) + ((p01 >> 16) & 0xFF) +
                     ((p11 >> 16) & 0xFF)) >>
                    2;
            int g = (((p00 >> 8) & 0xFF) + ((p10 >> 8) & 0xFF) + ((p01 >> 8) & 0xFF) +
                     ((p11 >> 8) & 0xFF)) >>
                    2;
            int b = ((p00 & 0xFF) + (p10 & 0xFF) + (p01 & 0xFF) + (p11 & 0xFF)) >> 2;

            // Perceptual Rec. 709 luminance
            float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;

            if (luma > thresh_luma) {
                float excess = (luma - thresh_luma) / (255.0f - thresh_luma + 1e-4f);
                uint8_t br = static_cast<uint8_t>(std::clamp(r * excess, 0.0f, 255.0f));
                uint8_t bg = static_cast<uint8_t>(std::clamp(g * excess, 0.0f, 255.0f));
                uint8_t bb = static_cast<uint8_t>(std::clamp(b * excess, 0.0f, 255.0f));
                bloom_buf0_[static_cast<size_t>(by) * bloom_w_ + bx] =
                    (0xFFu << 24) | (static_cast<uint32_t>(br) << 16) |
                    (static_cast<uint32_t>(bg) << 8) | bb;
            } else {
                bloom_buf0_[static_cast<size_t>(by) * bloom_w_ + bx] = 0;
            }
        }
    }
}

// Separable fast box/binomial blur on half-resolution buffer
void PostProcessor::blur_bloom_buffer() {
    // Pass 1: Horizontal blur (buf0 -> buf1)
    for (int y = 0; y < bloom_h_; ++y) {
        size_t row_offset = static_cast<size_t>(y) * bloom_w_;
        for (int x = 0; x < bloom_w_; ++x) {
            int xm2 = std::max(0, x - 2);
            int xm1 = std::max(0, x - 1);
            int xp1 = std::min(bloom_w_ - 1, x + 1);
            int xp2 = std::min(bloom_w_ - 1, x + 2);

            uint32_t c0 = bloom_buf0_[row_offset + xm2];
            uint32_t c1 = bloom_buf0_[row_offset + xm1];
            uint32_t c2 = bloom_buf0_[row_offset + x];
            uint32_t c3 = bloom_buf0_[row_offset + xp1];
            uint32_t c4 = bloom_buf0_[row_offset + xp2];

            // 1-4-6-4-1 binomial kernel (sum = 16)
            int r = (((c0 >> 16) & 0xFF) * 1 + ((c1 >> 16) & 0xFF) * 4 + ((c2 >> 16) & 0xFF) * 6 +
                     ((c3 >> 16) & 0xFF) * 4 + ((c4 >> 16) & 0xFF) * 1) >>
                    4;
            int g = (((c0 >> 8) & 0xFF) * 1 + ((c1 >> 8) & 0xFF) * 4 + ((c2 >> 8) & 0xFF) * 6 +
                     ((c3 >> 8) & 0xFF) * 4 + ((c4 >> 8) & 0xFF) * 1) >>
                    4;
            int b = ((c0 & 0xFF) * 1 + (c1 & 0xFF) * 4 + (c2 & 0xFF) * 6 + (c3 & 0xFF) * 4 +
                     (c4 & 0xFF) * 1) >>
                    4;

            bloom_buf1_[row_offset + x] = (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) |
                                          (static_cast<uint32_t>(g) << 8) | b;
        }
    }

    // Pass 2: Vertical blur (buf1 -> buf0)
    for (int y = 0; y < bloom_h_; ++y) {
        int ym2 = std::max(0, y - 2);
        int ym1 = std::max(0, y - 1);
        int yp1 = std::min(bloom_h_ - 1, y + 1);
        int yp2 = std::min(bloom_h_ - 1, y + 2);

        size_t off_m2 = static_cast<size_t>(ym2) * bloom_w_;
        size_t off_m1 = static_cast<size_t>(ym1) * bloom_w_;
        size_t off_0 = static_cast<size_t>(y) * bloom_w_;
        size_t off_p1 = static_cast<size_t>(yp1) * bloom_w_;
        size_t off_p2 = static_cast<size_t>(yp2) * bloom_w_;

        for (int x = 0; x < bloom_w_; ++x) {
            uint32_t c0 = bloom_buf1_[off_m2 + x];
            uint32_t c1 = bloom_buf1_[off_m1 + x];
            uint32_t c2 = bloom_buf1_[off_0 + x];
            uint32_t c3 = bloom_buf1_[off_p1 + x];
            uint32_t c4 = bloom_buf1_[off_p2 + x];

            int r = (((c0 >> 16) & 0xFF) * 1 + ((c1 >> 16) & 0xFF) * 4 + ((c2 >> 16) & 0xFF) * 6 +
                     ((c3 >> 16) & 0xFF) * 4 + ((c4 >> 16) & 0xFF) * 1) >>
                    4;
            int g = (((c0 >> 8) & 0xFF) * 1 + ((c1 >> 8) & 0xFF) * 4 + ((c2 >> 8) & 0xFF) * 6 +
                     ((c3 >> 8) & 0xFF) * 4 + ((c4 >> 8) & 0xFF) * 1) >>
                    4;
            int b = ((c0 & 0xFF) * 1 + (c1 & 0xFF) * 4 + (c2 & 0xFF) * 6 + (c3 & 0xFF) * 4 +
                     (c4 & 0xFF) * 1) >>
                    4;

            bloom_buf0_[off_0 + x] = (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) |
                                     (static_cast<uint32_t>(g) << 8) | b;
        }
    }
}

void PostProcessor::composite_effects(Framebuffer& fb, const PostProcessConfig& config) {
    uint32_t* color_buf = fb.color_data();
    const float* depth_buf = fb.depth_data();
    int w = fb.width();
    int h = fb.height();

    float fog_r = ((config.fog_color >> 16) & 0xFF) / 255.0f;
    float fog_g = ((config.fog_color >> 8) & 0xFF) / 255.0f;
    float fog_b = (config.fog_color & 0xFF) / 255.0f;

    float fog_span = config.fog_far - config.fog_near;
    if (fog_span <= 0.001f)
        fog_span = 0.001f;
    float inv_fog_span = 1.0f / fog_span;

    float near_z = config.camera_near;
    float far_z = config.camera_far;

    float cx = w * 0.5f;
    float cy = h * 0.5f;
    float max_dist_sq = cx * cx + cy * cy;

    for (int y = 0; y < h; ++y) {
        float dy = (static_cast<float>(y) - cy);
        int by = std::min(y / 2, bloom_h_ - 1);
        size_t bloom_row = static_cast<size_t>(by) * bloom_w_;
        size_t row_idx = static_cast<size_t>(y) * w;

        for (int x = 0; x < w; ++x) {
            size_t idx = row_idx + x;
            uint32_t orig_c = color_buf[idx];
            float depth = depth_buf[idx];

            float r = ((orig_c >> 16) & 0xFF) / 255.0f;
            float g = ((orig_c >> 8) & 0xFF) / 255.0f;
            float b = (orig_c & 0xFF) / 255.0f;

            // 1. Atmospheric Distance Fog
            if (config.enable_fog) {
                float linear_dist;
                if (depth >= 0.9999f) {
                    linear_dist = far_z;
                } else {
                    // Linearize perspective depth [0, 1] to view distance
                    linear_dist = (near_z * far_z) / (far_z - depth * (far_z - near_z));
                }

                float fog_factor =
                    std::clamp((linear_dist - config.fog_near) * inv_fog_span, 0.0f, 1.0f);
                r = r + (fog_r - r) * fog_factor;
                g = g + (fog_g - g) * fog_factor;
                b = b + (fog_b - b) * fog_factor;
            }

            // 2. Additive Bloom Glow
            if (config.enable_bloom) {
                int bx = std::min(x / 2, bloom_w_ - 1);
                uint32_t bloom_c = bloom_buf0_[bloom_row + bx];
                float bl_r = ((bloom_c >> 16) & 0xFF) / 255.0f;
                float bl_g = ((bloom_c >> 8) & 0xFF) / 255.0f;
                float bl_b = (bloom_c & 0xFF) / 255.0f;

                r += bl_r * config.bloom_intensity;
                g += bl_g * config.bloom_intensity;
                b += bl_b * config.bloom_intensity;
            }

            // 3. Color Grading & Contrast
            if (config.enable_color_grade) {
                // Exposure
                r *= config.exposure;
                g *= config.exposure;
                b *= config.exposure;

                // Contrast: S-curve centered at 0.5
                r = (r - 0.5f) * config.contrast + 0.5f;
                g = (g - 0.5f) * config.contrast + 0.5f;
                b = (b - 0.5f) * config.contrast + 0.5f;

                // Saturation
                float gray = 0.2126f * r + 0.7152f * g + 0.0722f * b;
                r = gray + (r - gray) * config.saturation;
                g = gray + (g - gray) * config.saturation;
                b = gray + (b - gray) * config.saturation;
            }

            // 4. Cinematic Vignette
            if (config.enable_vignette) {
                float dx = (static_cast<float>(x) - cx);
                float dist_sq = (dx * dx + dy * dy) / max_dist_sq;
                float vig = 1.0f - smoothstep(config.vignette_radius,
                                              config.vignette_radius + config.vignette_softness,
                                              std::sqrt(dist_sq));
                r *= vig;
                g *= vig;
                b *= vig;
            }

            // Reinhard Tone Mapping to avoid clipping on hot highlights
            r = r / (1.0f + r);
            g = g / (1.0f + g);
            b = b / (1.0f + b);

            uint8_t out_r = static_cast<uint8_t>(std::clamp(r * 255.0f, 0.0f, 255.0f));
            uint8_t out_g = static_cast<uint8_t>(std::clamp(g * 255.0f, 0.0f, 255.0f));
            uint8_t out_b = static_cast<uint8_t>(std::clamp(b * 255.0f, 0.0f, 255.0f));

            color_buf[idx] = (0xFFu << 24) | (static_cast<uint32_t>(out_r) << 16) |
                             (static_cast<uint32_t>(out_g) << 8) | out_b;
        }
    }
}

void PostProcessor::apply(Framebuffer& fb, const PostProcessConfig& config) {
    if (fb.width() != width_ || fb.height() != height_) {
        init(fb.width(), fb.height());
    }

    if (config.enable_bloom) {
        extract_bright_pixels(fb, config.bloom_threshold);
        for (int i = 0; i < config.blur_iterations; ++i) {
            blur_bloom_buffer();
        }
    }

    composite_effects(fb, config);
}

} // namespace prism::render
