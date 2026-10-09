#include "nu/platform/window.hpp"

#include "nu/core/assert.hpp"
#include "nu/core/log.hpp"
#include "nu/platform/input.hpp"

#include <stdexcept>

namespace nu {

Window::Window(const WindowDesc& desc) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS)) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    const SDL_WindowFlags flags =
        SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY |
        (desc.resizable ? SDL_WINDOW_RESIZABLE : static_cast<SDL_WindowFlags>(0)) |
        (desc.fullscreen ? SDL_WINDOW_FULLSCREEN : static_cast<SDL_WindowFlags>(0));

    window_ = SDL_CreateWindow(desc.title.c_str(), desc.width, desc.height, flags);
    if (!window_) {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    width_ = desc.width;
    height_ = desc.height;
    NU_LOG_INFO("Window created {}x{} — {}", width_, height_, desc.title);
}

Window::~Window() {
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void* Window::nativeHandle() const {
#if defined(_WIN32)
    return SDL_GetPointerProperty(SDL_GetWindowProperties(window_), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
    return nullptr;
#endif
}

void Window::onResize(int w, int h) {
    if (w <= 0 || h <= 0) {
        return;
    }
    if (w != width_ || h != height_) {
        width_ = w;
        height_ = h;
        resized_ = true;
        NU_LOG_DEBUG("Window resized to {}x{}", width_, height_);
    }
}

bool Window::pumpEvents() {
    // Events are handled by Application with Input; this method kept for simple apps.
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            shouldClose_ = true;
        } else if (e.type == SDL_EVENT_WINDOW_RESIZED) {
            onResize(e.window.data1, e.window.data2);
        } else if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            onResize(e.window.data1, e.window.data2);
        }
    }
    return !shouldClose_;
}

} // namespace nu
