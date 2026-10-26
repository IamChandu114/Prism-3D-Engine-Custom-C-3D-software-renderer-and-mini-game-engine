#include <cstring>
#include <prism/core/window.hpp>
#include <prism/render/framebuffer.hpp>

#if defined(PRISM_HEADLESS)
// Headless mode: no SDL
#else
#include <SDL2/SDL.h>
#endif

namespace prism::core {

Window::Window(const std::string& title, int width, int height, bool headless)
    : width_(width), height_(height), headless_(headless), is_running_(true) {
#if !defined(PRISM_HEADLESS)
    if (!headless_) {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            is_running_ = false;
            return;
        }
        window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   width_, height_, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        if (!window_) {
            is_running_ = false;
            return;
        }
        renderer_ =
            SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer_) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
            is_running_ = false;
            return;
        }
        texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, width_, height_);
        if (!texture_) {
            SDL_DestroyRenderer(renderer_);
            SDL_DestroyWindow(window_);
            renderer_ = nullptr;
            window_ = nullptr;
            is_running_ = false;
            return;
        }
    }
#endif
}

Window::~Window() {
#if !defined(PRISM_HEADLESS)
    if (!headless_) {
        if (texture_) {
            SDL_DestroyTexture(texture_);
        }
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
        }
        if (window_) {
            SDL_DestroyWindow(window_);
        }
        SDL_Quit();
    }
#endif
}

Window::Window(Window&& other) noexcept
    : width_(other.width_), height_(other.height_), headless_(other.headless_),
      is_running_(other.is_running_), window_(other.window_), renderer_(other.renderer_),
      texture_(other.texture_) {
    other.window_ = nullptr;
    other.renderer_ = nullptr;
    other.texture_ = nullptr;
    other.is_running_ = false;
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        width_ = other.width_;
        height_ = other.height_;
        headless_ = other.headless_;
        is_running_ = other.is_running_;
        window_ = other.window_;
        renderer_ = other.renderer_;
        texture_ = other.texture_;
        other.window_ = nullptr;
        other.renderer_ = nullptr;
        other.texture_ = nullptr;
        other.is_running_ = false;
    }
    return *this;
}

void Window::poll_events() noexcept {
#if !defined(PRISM_HEADLESS)
    if (headless_)
        return;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            is_running_ = false;
        } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
            is_running_ = false;
        }
    }
#endif
}

void Window::present(const render::Framebuffer& fb) noexcept {
#if !defined(PRISM_HEADLESS)
    if (headless_ || !texture_)
        return;
    void* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch) == 0) {
        const int row_bytes = fb.width() * 4;
        const uint8_t* src = reinterpret_cast<const uint8_t*>(fb.color_data());
        uint8_t* dst = static_cast<uint8_t*>(pixels);
        // Re-pack RGBA32 -> ARGB8888 for SDL
        // Our format: R at bits 23-16, G at bits 15-8, B at bits 7-0, (A=FF at bits 31-24)
        // SDL ARGB8888: A at bits 31-24, R at 23-16, G at 15-8, B at 7-0 (same layout)
        for (int y = 0; y < fb.height(); ++y) {
            std::memcpy(dst + static_cast<size_t>(y) * pitch,
                        src + static_cast<size_t>(y) * row_bytes, static_cast<size_t>(row_bytes));
        }
        SDL_UnlockTexture(texture_);
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);
    }
#endif
}

} // namespace prism::core
