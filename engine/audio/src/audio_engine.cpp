#include "nu/audio/audio_engine.hpp"

#include "nu/core/log.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <algorithm>
#include <vector>

namespace nu {

struct AudioEngine::Impl {
    ma_engine engine{};
    bool started = false;
    std::vector<ma_sound*> active;
};

AudioEngine::AudioEngine() = default;

AudioEngine::~AudioEngine() {
    shutdown();
}

void AudioEngine::init() {
    impl_ = new Impl();
    if (ma_engine_init(nullptr, &impl_->engine) != MA_SUCCESS) {
        NU_LOG_ERROR("miniaudio engine init failed");
        delete impl_;
        impl_ = nullptr;
        return;
    }
    impl_->started = true;
    ready_ = true;
    NU_LOG_INFO("Audio engine ready");
}

void AudioEngine::shutdown() {
    if (impl_) {
        for (ma_sound* sound : impl_->active) {
            if (sound) {
                ma_sound_uninit(sound);
                delete sound;
            }
        }
        impl_->active.clear();
        if (impl_->started) {
            ma_engine_uninit(&impl_->engine);
        }
        delete impl_;
        impl_ = nullptr;
    }
    ready_ = false;
}

void AudioEngine::update() {
    if (!impl_) {
        return;
    }
    auto& active = impl_->active;
    for (std::size_t i = 0; i < active.size();) {
        ma_sound* sound = active[i];
        if (!sound || !ma_sound_is_playing(sound)) {
            if (sound) {
                ma_sound_uninit(sound);
                delete sound;
            }
            active[i] = active.back();
            active.pop_back();
            continue;
        }
        ++i;
    }
}

void AudioEngine::playSound(const std::filesystem::path& path, float pitch) {
    if (!ready_ || !impl_) {
        return;
    }
    if (!std::filesystem::exists(path)) {
        NU_LOG_WARN("Audio file missing: {}", path.string());
        return;
    }

    update();

    auto* sound = new ma_sound();
    const ma_result result = ma_sound_init_from_file(&impl_->engine, path.string().c_str(),
                                                     MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_ASYNC, nullptr, nullptr,
                                                     sound);
    if (result != MA_SUCCESS) {
        NU_LOG_ERROR("Failed to load sound {}: {}", path.string(), static_cast<int>(result));
        delete sound;
        return;
    }

    const float safePitch = pitch > 0.01f ? pitch : 1.0f;
    ma_sound_set_pitch(sound, safePitch);
    if (ma_sound_start(sound) != MA_SUCCESS) {
        NU_LOG_ERROR("Failed to start sound {}", path.string());
        ma_sound_uninit(sound);
        delete sound;
        return;
    }

    impl_->active.push_back(sound);
}

void AudioEngine::setMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
    if (ready_ && impl_) {
        ma_engine_set_volume(&impl_->engine, masterVolume_);
    }
}

} // namespace nu
