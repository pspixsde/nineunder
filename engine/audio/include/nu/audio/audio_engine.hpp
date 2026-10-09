#pragma once

#include <string>

namespace nu {

/// Thin miniaudio wrapper — init/shutdown for M0–M3; playback hooks later.
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    void init();
    void shutdown();

    [[nodiscard]] bool ready() const { return ready_; }

private:
    struct Impl;
    Impl* impl_ = nullptr;
    bool ready_ = false;
};

} // namespace nu
