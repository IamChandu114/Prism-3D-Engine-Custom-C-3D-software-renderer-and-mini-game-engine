#pragma once

#include <prism/render/framebuffer.hpp>
#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace prism::core {

class Window {
public:
    Window(const std::string& title, int width, int height, bool headless = false);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept;
    Window& operator=(Window&&) noexcept;

    [[nodiscard]] bool is_running() const noexcept { return is_running_; }
    [[nodiscard]] bool is_headless() const noexcept { return headless_; }
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }

    void poll_events() noexcept;
    void present(const render::Framebuffer& fb) noexcept;
    void close() noexcept { is_running_ = false; }

private:
    int width_{0};
    int height_{0};
    bool headless_{false};
    bool is_running_{false};
    SDL_Window* window_{nullptr};
    SDL_Renderer* renderer_{nullptr};
    SDL_Texture* texture_{nullptr};
};

} // namespace prism::core
