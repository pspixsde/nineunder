#pragma once

#include "nu/application.hpp"
#include "nu/render/mesh.hpp"

#include <vector>

namespace nine {

class GameApp final : public nu::Application {
public:
    GameApp();

protected:
    void onInit() override;
    void onShutdown() override;
    void onFixedUpdate(float dt) override;
    void onRender(float alpha) override;

private:
    void buildArenaOne();

    nu::GpuMesh floorMesh_{};
    nu::GpuMesh cubeMesh_{};
    nu::GpuMesh playerMesh_{};
    nu::GpuMesh wallMesh_{};
    std::vector<nu::DrawItem> staticDraws_;
    bool meshesReady_ = false;
};

} // namespace nine
