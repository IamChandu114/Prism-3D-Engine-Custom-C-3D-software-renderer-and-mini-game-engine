#pragma once

#include <cstdint>

namespace prism::render {

// Simple embedded 5x7 ASCII bitmap font (characters 32 ' ' through 126 '~')
// Each character is 5 columns wide, 7 rows tall, encoded as 5 bytes (1 byte per column).
struct BitmapFont {
    static constexpr int kCharWidth = 5;
    static constexpr int kCharHeight = 7;

    [[nodiscard]] static const uint8_t* get_glyph(char c) noexcept {
        if (c < 32 || c > 126) {
            c = '?';
        }
        return &kFontData[(c - 32) * 5];
    }

private:
    static const uint8_t kFontData[];
};

} // namespace prism::render
