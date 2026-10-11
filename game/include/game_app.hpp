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
    Playing,
    Paused,
    Death
};

enum class ProjectileOwner {
    Player,
    Imp
};

enum class ImpAiState {
    Roam,
    Aggro
};

struct ImpEnemy {
    nu::Vec3 position{18.0f, 0.0f, 14.0f};
    nu::Vec3 spawnPosition{18.0f, 0.0f, 14.0f};
    nu::Vec3 roamTarget{18.0f, 0.0f, 14.0f};
    float hp = 360.0f;
    float maxHp = 360.0f;
    bool alive = true;
    bool aggroed = false;
    float hitFlashTimer = 0.0f;
    float attackCooldown = 0.0f;
    float roamRetargetTimer = 0.0f;
    float noticeTimer = 0.0f;
    ImpAiState ai = ImpAiState::Roam;
};

struct Projectile {
    nu::Vec3 position{};
    nu::Vec3 velocity{};
    float lifetime = 0.0f;
    float radius = 0.18f;
    float damage = 60.0f;
    float gravity = 0.0f;
    ProjectileOwner owner = ProjectileOwner::Player;
    float trailEmitTimer = 0.0f;
};

struct TrailParticle {
    nu::Vec3 position{};
    float age = 0.0f;
    float lifetime = 0.35f;
    float scale = 0.14f;
};

struct MeleeSlashVfx {
    nu::Vec3 position{};
    nu::Vec3 forward{};
    float age = 0.0f;
    float lifetime = 0.22f;
};

struct DamageNumber {
    nu::Vec3 worldPos{};
    float amount = 0.0f;
    float age = 0.0f;
    float lifetime = 0.85f;
};

struct PlayerCombat {
    float hp = 500.0f;
    float maxHp = 500.0f;
    int shotsInClip = 0;
    int clipSize = 6;
    float fireCooldown = 0.0f;
    float fireInterval = 0.35f;
    float reloadTimer = 0.0f;
    float reloadDuration = 1.75f;
    bool reloading = false;
    float meleeCooldown = 0.0f;
    float meleeCooldownDuration = 3.0f;
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
    void pauseGame();
    void resumeGame();
    void killPlayer();
    void resetEncounter();
    void killImp();
    void respawnImp();
    void pickImpRoamTarget();
    void tryFireProjectile();
    void tryMeleeAttack();
    void updateCombat(float dt);
    void updateImpAi(float dt);
    void updateProjectiles(float dt);
    void updateVfx(float dt);
    void updateDamageNumbers(float dt);
    void damageImp(float amount, const nu::Vec3& hitPos);
    void damagePlayer(float amount, const nu::Vec3& hitPos);
    void spawnImpSpit();
    void drawMainMenu();
    void drawPauseMenu();
    void drawDeathMenu();
    void drawGameplayHud();
    void drawFpsCounter();
    void drawPlayerHpBar();
    void drawAmmoPips();
    void drawAbilitySlots();
    void drawCrosshair();
    void drawImpHpBar();
    void drawDamageNumbers();

    [[nodiscard]] bool isSimulating() const;
    [[nodiscard]] nu::Vec3 playerFlatForward();

    AppScreen screen_ = AppScreen::MainMenu;

    nu::GpuMesh floorMesh_{};
    nu::GpuMesh cubeMesh_{};
    nu::GpuMesh playerMesh_{};
    nu::GpuMesh wallMesh_{};
    nu::GpuMesh impMesh_{};
    nu::GpuMesh projectileMesh_{};
    std::vector<nu::DrawItem> staticDraws_;
    bool meshesReady_ = false;

    std::optional<nu::ModelAsset> urnProp_;
    std::vector<nu::Mat4> urnTransforms_;
    nu::AssetHotReload hotReload_;

    ImpEnemy imp_{};
    PlayerCombat player_{};
    std::vector<Projectile> projectiles_;
    std::vector<TrailParticle> trailParticles_;
    std::vector<MeleeSlashVfx> meleeVfx_;
    std::vector<DamageNumber> damageNumbers_;
    std::mt19937 rng_{std::random_device{}()};
};

} // namespace nine
