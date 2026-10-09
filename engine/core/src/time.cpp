#include "nu/core/time.hpp"

#include <SDL3/SDL.h>

#include <algorithm>

namespace nu {

double wallSeconds() {
    return static_cast<double>(SDL_GetTicksNS()) / 1'000'000'000.0;
}

Clock::Clock() {
    reset();
}

void Clock::reset() {
    startSeconds_ = wallSeconds();
    lastSeconds_ = startSeconds_;
    totalSeconds_ = 0.0;
    lastDelta_ = 0.0f;
    fps_ = 0.0f;
    fpsAccum_ = 0.0f;
    fpsFrames_ = 0;
}

float Clock::tick() {
    const double now = wallSeconds();
    double dt = now - lastSeconds_;
    lastSeconds_ = now;
    totalSeconds_ = now - startSeconds_;

    // Clamp large hitches (debug breakpoints, window drags).
    dt = std::min(dt, 0.1);
    lastDelta_ = static_cast<float>(dt);

    fpsAccum_ += lastDelta_;
    ++fpsFrames_;
    if (fpsAccum_ >= 0.5f) {
        fps_ = static_cast<float>(fpsFrames_) / fpsAccum_;
        fpsAccum_ = 0.0f;
        fpsFrames_ = 0;
    }

    return lastDelta_;
}

} // namespace nu
