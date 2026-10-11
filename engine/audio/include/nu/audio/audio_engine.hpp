#pragma once

#include <filesystem>
#include <string>

namespace nu {

/// Thin miniaudio wrapper with one-shot SFX playback.
class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    void init();
    void shutdown();

    /// Play a WAV/OGG/etc. from disk. `pitch` of 1.0 is normal (e.g. 0.9–1.1 for variety).
    void playSound(const std::filesystem::path& path, float pitch = 1.0f);

    /// Release finished one-shot sounds (call once per frame while audio is used).
    void update();

    [[nodiscard]] bool ready() const { return ready_; }

private:
    struct Impl;
    Impl* impl_ = nullptr;
    bool ready_ = false;
};

} // namespace nu
