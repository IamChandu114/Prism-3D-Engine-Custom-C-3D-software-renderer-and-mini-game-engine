#include <gtest/gtest.h>
#include <prism/render/framebuffer.hpp>

using namespace prism::render;

TEST(FramebufferTest, CreationAndClear) {
    Framebuffer fb(100, 100);
    EXPECT_EQ(fb.width(), 100);
    EXPECT_EQ(fb.height(), 100);

    uint32_t clear_color = 0xFF112233;
    fb.clear(clear_color, 1.0f);

    EXPECT_EQ(fb.get_pixel(0, 0), clear_color);
    EXPECT_EQ(fb.get_pixel(99, 99), clear_color);
    EXPECT_NEAR(fb.get_depth(50, 50), 1.0f, 1e-5f);
}

TEST(FramebufferTest, SetPixelBoundsCheck) {
    Framebuffer fb(50, 50);
    fb.clear(0x00000000);

    fb.set_pixel(10, 20, 0xFFFFFFFF);
    EXPECT_EQ(fb.get_pixel(10, 20), 0xFFFFFFFF);

    // Out of bounds should safely no-op without segfault
    fb.set_pixel(-1, 20, 0xFFFFFFFF);
    fb.set_pixel(50, 20, 0xFFFFFFFF);
    fb.set_pixel(10, -5, 0xFFFFFFFF);
    fb.set_pixel(10, 60, 0xFFFFFFFF);
    EXPECT_EQ(fb.get_pixel(0, 0), 0x00000000);
}

TEST(FramebufferTest, DepthBufferTest) {
    Framebuffer fb(10, 10);
    fb.clear(0x00000000, 1.0f);

    // Write closer depth (0.5 < 1.0) -> should pass
    bool passed1 = fb.set_pixel_depth(5, 5, 0.5f, 0xFF00FF00);
    EXPECT_TRUE(passed1);
    EXPECT_EQ(fb.get_pixel(5, 5), 0xFF00FF00);
    EXPECT_NEAR(fb.get_depth(5, 5), 0.5f, 1e-5f);

    // Write farther depth (0.8 > 0.5) -> should fail depth test
    bool passed2 = fb.set_pixel_depth(5, 5, 0.8f, 0xFFFF0000);
    EXPECT_FALSE(passed2);
    EXPECT_EQ(fb.get_pixel(5, 5), 0xFF00FF00); // Unchanged
    EXPECT_NEAR(fb.get_depth(5, 5), 0.5f, 1e-5f);
}

TEST(FramebufferTest, BresenhamHorizontalAndVerticalLines) {
    Framebuffer fb(20, 20);
    fb.clear(0x00000000);

    // Horizontal line y = 5, x from 2 to 8
    fb.draw_line(2, 5, 8, 5, 0xFFFFFFFF);
    for (int x = 2; x <= 8; ++x) {
        EXPECT_EQ(fb.get_pixel(x, 5), 0xFFFFFFFF) << "Mismatch at x=" << x;
    }
    EXPECT_EQ(fb.get_pixel(1, 5), 0x00000000);
    EXPECT_EQ(fb.get_pixel(9, 5), 0x00000000);

    // Vertical line x = 12, y from 3 to 9
    fb.draw_line(12, 3, 12, 9, 0xFFFFFFFF);
    for (int y = 3; y <= 9; ++y) {
        EXPECT_EQ(fb.get_pixel(12, y), 0xFFFFFFFF) << "Mismatch at y=" << y;
    }
    EXPECT_EQ(fb.get_pixel(12, 2), 0x00000000);
    EXPECT_EQ(fb.get_pixel(12, 10), 0x00000000);
}

TEST(FramebufferTest, BresenhamDiagonalAndClipping) {
    Framebuffer fb(30, 30);
    fb.clear(0x00000000);

    // Diagonal line from (-10, -10) to (40, 40)
    // Must be clipped to [0, 29] without out-of-bounds memory corruption
    fb.draw_line(-10, -10, 40, 40, 0xFFAABBCC);
    EXPECT_EQ(fb.get_pixel(0, 0), 0xFFAABBCC);
    EXPECT_EQ(fb.get_pixel(15, 15), 0xFFAABBCC);
    EXPECT_EQ(fb.get_pixel(29, 29), 0xFFAABBCC);
}
