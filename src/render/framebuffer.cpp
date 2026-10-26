#include <algorithm>
#include <cmath>
#include <fstream>
#include <prism/render/font.hpp>
#include <prism/render/framebuffer.hpp>

namespace prism::render {

namespace {
// Cohen-Sutherland outcode flags
constexpr int kInside = 0; // 0000
constexpr int kLeft = 1;   // 0001
constexpr int kRight = 2;  // 0010
constexpr int kBottom = 4; // 0100
constexpr int kTop = 8;    // 1000

int compute_outcode(int x, int y, int x_min, int y_min, int x_max, int y_max) noexcept {
    int code = kInside;
    if (x < x_min) {
        code |= kLeft;
    } else if (x > x_max) {
        code |= kRight;
    }
    if (y < y_min) {
        code |= kTop;
    } else if (y > y_max) {
        code |= kBottom;
    }
    return code;
}

bool clip_line(int& x0, int& y0, int& x1, int& y1, int width, int height) noexcept {
    int x_min = 0;
    int y_min = 0;
    int x_max = width - 1;
    int y_max = height - 1;

    int outcode0 = compute_outcode(x0, y0, x_min, y_min, x_max, y_max);
    int outcode1 = compute_outcode(x1, y1, x_min, y_min, x_max, y_max);
    bool accept = false;

    while (true) {
        if ((outcode0 | outcode1) == 0) {
            // Both points inside
            accept = true;
            break;
        } else if ((outcode0 & outcode1) != 0) {
            // Both points share an outside zone -> completely offscreen
            break;
        } else {
            int x = 0;
            int y = 0;
            int outcode_out = outcode0 != 0 ? outcode0 : outcode1;

            if ((outcode_out & kBottom) != 0) {
                x = x0 + static_cast<int>(static_cast<int64_t>(x1 - x0) * (y_max - y0) / (y1 - y0));
                y = y_max;
            } else if ((outcode_out & kTop) != 0) {
                x = x0 + static_cast<int>(static_cast<int64_t>(x1 - x0) * (y_min - y0) / (y1 - y0));
                y = y_min;
            } else if ((outcode_out & kRight) != 0) {
                y = y0 + static_cast<int>(static_cast<int64_t>(y1 - y0) * (x_max - x0) / (x1 - x0));
                x = x_max;
            } else if ((outcode_out & kLeft) != 0) {
                y = y0 + static_cast<int>(static_cast<int64_t>(y1 - y0) * (x_min - x0) / (x1 - x0));
                x = x_min;
            }

            if (outcode_out == outcode0) {
                x0 = x;
                y0 = y;
                outcode0 = compute_outcode(x0, y0, x_min, y_min, x_max, y_max);
            } else {
                x1 = x;
                y1 = y;
                outcode1 = compute_outcode(x1, y1, x_min, y_min, x_max, y_max);
            }
        }
    }
    return accept;
}
} // namespace

Framebuffer::Framebuffer(int width, int height)
    : width_(width), height_(height),
      color_buffer_(static_cast<size_t>(width) * height, 0x00000000),
      depth_buffer_(static_cast<size_t>(width) * height, 1.0f) {}

void Framebuffer::clear(uint32_t color, float depth) noexcept {
    std::fill(color_buffer_.begin(), color_buffer_.end(), color);
    std::fill(depth_buffer_.begin(), depth_buffer_.end(), depth);
}

void Framebuffer::draw_line(int x0, int y0, int x1, int y1, uint32_t color) noexcept {
    if (!clip_line(x0, y0, x1, y1, width_, height_)) {
        return;
    }

    int dx = std::abs(x1 - x0);
    int dy = -std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        set_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Framebuffer::draw_char(int start_x, int start_y, char c, uint32_t color, int scale) noexcept {
    const uint8_t* glyph = BitmapFont::get_glyph(c);
    for (int col = 0; col < BitmapFont::kCharWidth; ++col) {
        uint8_t line = glyph[col];
        for (int row = 0; row < BitmapFont::kCharHeight; ++row) {
            if ((line & (1 << row)) != 0) {
                for (int sy = 0; sy < scale; ++sy) {
                    for (int sx = 0; sx < scale; ++sx) {
                        set_pixel(start_x + col * scale + sx, start_y + row * scale + sy, color);
                    }
                }
            }
        }
    }
}

void Framebuffer::draw_string(int x, int y, const std::string& text, uint32_t color,
                              int scale) noexcept {
    int cur_x = x;
    int cur_y = y;
    int spacing = (BitmapFont::kCharWidth + 1) * scale;
    int line_height = (BitmapFont::kCharHeight + 2) * scale;

    for (char c : text) {
        if (c == '\n') {
            cur_x = x;
            cur_y += line_height;
        } else {
            draw_char(cur_x, cur_y, c, color, scale);
            cur_x += spacing;
        }
    }
}

bool Framebuffer::save_ppm(const std::string& filepath) const {
    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
        return false;
    }

    out << "P6\n" << width_ << " " << height_ << "\n255\n";
    std::vector<uint8_t> rgb_bytes(static_cast<size_t>(width_) * height_ * 3);

    for (size_t i = 0; i < color_buffer_.size(); ++i) {
        uint32_t pixel = color_buffer_[i];
        // Standard RGBA32 format: (R = pixel & 0xFF), (G = (pixel >> 8) & 0xFF), (B = (pixel >> 16)
        // & 0xFF)
        rgb_bytes[i * 3 + 0] = static_cast<uint8_t>((pixel >> 16) & 0xFF); // R
        rgb_bytes[i * 3 + 1] = static_cast<uint8_t>((pixel >> 8) & 0xFF);  // G
        rgb_bytes[i * 3 + 2] = static_cast<uint8_t>(pixel & 0xFF);         // B
    }

    out.write(reinterpret_cast<const char*>(rgb_bytes.data()),
              static_cast<std::streamsize>(rgb_bytes.size()));
    return true;
}

} // namespace prism::render
