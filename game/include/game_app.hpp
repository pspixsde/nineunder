#pragma once

#include "nu/application.hpp"
#include "nu/assets/asset_hot_reload.hpp"
#include "nu/assets/model_asset.hpp"
#include "nu/render/mesh.hpp"

#include <optional>
#include <random>
#include <vector>

namespace nine {

enum class AppScreen {
    MainMenu,
    Playing
};

struct ImpEnemy {
    nu::Vec3 position{5.0f, 0.0f, 5.0f};
    float hp = 360.0f;
    float maxHp = 360.0f;
    bool alive = true;
};

class GameApp final : public nu::Application {
public:
    GameApp();

protected:
    void onInit() override;
    void onShutdown() override;
    void onFixedUpdate(float dt) override;
    void onVariableUpdate(float dt) override;
    void onRender(float alpha) override;
    void onEscapePressed() override;

private:
    void buildArenaOne();
    void tryLoadUrns();
    void setupHotReload();
    void enterMainMenu();
    void startPlay();
    void killImp();
    void respawnImp();
    void drawMainMenu();
    void drawGameplayHud();
    void drawImpHpBar();

    AppScreen screen_ = AppScreen::MainMenu;

    nu::GpuMesh floorMesh_{};
    nu::GpuMesh cubeMesh_{};
    nu::GpuMesh playerMesh_{};
    nu::GpuMesh wallMesh_{};
    nu::GpuMesh impMesh_{};
    std::vector<nu::DrawItem> staticDraws_;
    bool meshesReady_ = false;

    std::optional<nu::ModelAsset> urnProp_;
    std::vector<nu::Mat4> urnTransforms_;
    nu::AssetHotReload hotReload_;

    ImpEnemy imp_{};
    std::mt19937 rng_{std::random_device{}()};
};

} // namespace nine
