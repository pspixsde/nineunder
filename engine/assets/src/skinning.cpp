#include "nu/assets/skinning.hpp"

#include <algorithm>
#include <cmath>

namespace nu {
namespace {

int findKey(const std::vector<float>& times, float t) {
    if (times.empty()) {
        return -1;
    }
    if (t <= times.front()) {
        return 0;
    }
    for (int i = 0; i + 1 < static_cast<int>(times.size()); ++i) {
        if (t < times[static_cast<size_t>(i + 1)]) {
            return i;
        }
    }
    return static_cast<int>(times.size()) - 1;
}

} // namespace

void AnimationSampler::sampleLocalPose(const Skeleton& skeleton, std::vector<Mat4>& localPose) const {
    localPose.assign(skeleton.joints.size(), Mat4{1.0f});
    if (!clip_) {
        return;
    }
    float t = time_;
    if (clip_->duration > 0.0f) {
        t = std::fmod(t, clip_->duration);
        if (t < 0.0f) {
            t += clip_->duration;
        }
    }

    std::vector<Vec3> translations(skeleton.joints.size(), Vec3{0.0f});
    std::vector<Quat> rotations(skeleton.joints.size(), Quat{1.0f, 0.0f, 0.0f, 0.0f});
    std::vector<Vec3> scales(skeleton.joints.size(), Vec3{1.0f});

    for (const AnimationChannel& ch : clip_->channels) {
        if (ch.jointIndex < 0 || static_cast<size_t>(ch.jointIndex) >= skeleton.joints.size()) {
            continue;
        }
        const int key = findKey(ch.times, t);
        if (key < 0) {
            continue;
        }
        const size_t ji = static_cast<size_t>(ch.jointIndex);
        if (ch.path == AnimationChannel::Path::Translation && key * 3 + 2 < static_cast<int>(ch.values.size())) {
            translations[ji] = {ch.values[static_cast<size_t>(key) * 3 + 0], ch.values[static_cast<size_t>(key) * 3 + 1],
                                ch.values[static_cast<size_t>(key) * 3 + 2]};
        } else if (ch.path == AnimationChannel::Path::Scale && key * 3 + 2 < static_cast<int>(ch.values.size())) {
            scales[ji] = {ch.values[static_cast<size_t>(key) * 3 + 0], ch.values[static_cast<size_t>(key) * 3 + 1],
                          ch.values[static_cast<size_t>(key) * 3 + 2]};
        } else if (ch.path == AnimationChannel::Path::Rotation && key * 4 + 3 < static_cast<int>(ch.values.size())) {
            rotations[ji] = Quat{ch.values[static_cast<size_t>(key) * 4 + 3], ch.values[static_cast<size_t>(key) * 4 + 0],
                                 ch.values[static_cast<size_t>(key) * 4 + 1], ch.values[static_cast<size_t>(key) * 4 + 2]};
        }
    }

    for (size_t i = 0; i < skeleton.joints.size(); ++i) {
        localPose[i] = glm::translate(Mat4{1.0f}, translations[i]) * glm::mat4_cast(rotations[i]) *
                       glm::scale(Mat4{1.0f}, scales[i]);
    }
}

} // namespace nu
