#pragma once

#include "nu/math/math.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace nu {

/// CPU-side skinning / animation data (Phase 5). GPU skinning lands with a
/// SkinnedVertex path and joint-matrix UBO — see docs/CHARACTERS.md.

struct SkinnedVertex {
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
    Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    Vec4 tangent{1.0f, 0.0f, 0.0f, 1.0f};
    uint16_t joints[4]{};
    Vec4 weights{0.0f, 0.0f, 0.0f, 0.0f};
};

struct SkeletonJoint {
    std::string name;
    int parent = -1;
    Mat4 inverseBind{1.0f};
};

struct Skeleton {
    std::vector<SkeletonJoint> joints;
};

struct AnimationChannel {
    int jointIndex = -1;
    enum class Path { Translation, Rotation, Scale } path = Path::Translation;
    std::vector<float> times;
    std::vector<float> values; // 3 floats/key for T/S, 4 for quat XYZ W
};

struct AnimationClip {
    std::string name;
    float duration = 0.0f;
    std::vector<AnimationChannel> channels;
};

/// Placeholder sampler — replace with ozz-animation when the vcpkg port (or
/// third_party vendor) is wired. Samples nearest-key T/R/S into local matrices.
class AnimationSampler {
public:
    void setClip(const AnimationClip* clip) { clip_ = clip; }
    void setTime(float t) { time_ = t; }
    [[nodiscard]] float time() const { return time_; }
    [[nodiscard]] const AnimationClip* clip() const { return clip_; }

    /// Fills `localPose` with one Mat4 per joint (identity if missing channel).
    void sampleLocalPose(const Skeleton& skeleton, std::vector<Mat4>& localPose) const;

private:
    const AnimationClip* clip_ = nullptr;
    float time_ = 0.0f;
};

} // namespace nu
