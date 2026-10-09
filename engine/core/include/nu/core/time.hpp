#pragma once

namespace nu {

/// Fixed simulation rate (Hz) used by Application.
inline constexpr float kFixedHz = 60.0f;
inline constexpr float kFixedDt = 1.0f / kFixedHz;

class Clock {
public:
    Clock();

    /// Call once per frame. Returns wall delta seconds (clamped).
    float tick();

    [[nodiscard]] double totalSeconds() const { return totalSeconds_; }
    [[nodiscard]] float lastDelta() const { return lastDelta_; }
    [[nodiscard]] float fps() const { return fps_; }

    void reset();

private:
    double startSeconds_ = 0.0;
    double lastSeconds_ = 0.0;
    double totalSeconds_ = 0.0;
    float lastDelta_ = 0.0f;
    float fps_ = 0.0f;
    float fpsAccum_ = 0.0f;
    int fpsFrames_ = 0;
};

double wallSeconds();

} // namespace nu
