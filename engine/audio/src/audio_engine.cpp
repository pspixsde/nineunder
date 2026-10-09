#include "nu/audio/audio_engine.hpp"

#include "nu/core/log.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

namespace nu {

struct AudioEngine::Impl {
    ma_engine engine{};
    bool started = false;
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
        if (impl_->started) {
            ma_engine_uninit(&impl_->engine);
        }
        delete impl_;
        impl_ = nullptr;
    }
    ready_ = false;
}

} // namespace nu
