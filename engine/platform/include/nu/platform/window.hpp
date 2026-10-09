#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

namespace nu {

struct WindowDesc {
    std::string title = "Nine Under";
    int width = 1600;
    int height = 900;
    bool resizable = true;
    bool fullscreen = false;
};

class Window {
public:
    explicit Window(const WindowDesc& desc);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]] SDL_Window* sdlWindow() const { return window_; }
    [[nodiscard]] void* nativeHandle() const;
    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }
    [[nodiscard]] float aspect() const {
        return height_ > 0 ? static_cast<float>(width_) / static_cast<float>(height_) : 1.0f;
    }
    [[nodiscard]] bool shouldClose() const { return shouldClose_; }
    [[nodiscard]] bool resized() const { return resized_; }

    void clearResized() { resized_ = false; }
    void requestClose() { shouldClose_ = true; }
    void onResize(int w, int h);

    /// Poll SDL events into this window + input. Returns false if quit requested.
    bool pumpEvents();

private:
    SDL_Window* window_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    bool shouldClose_ = false;
    bool resized_ = false;
};

} // namespace nu
