#pragma once

#include "map_data.hpp"
#include "nu/application.hpp"
#include "nu/assets/asset_hot_reload.hpp"
#include "nu/assets/model_asset.hpp"
#include "nu/render/mesh.hpp"
#include "nu/render/vulkan_device.hpp"

#include <array>
#include <optional>
#include <random>
#include <vector>

namespace nine {

enum class AppScreen {
    MainMenu,
    Playing,
    Paused,
    Death,
    Settings,
    MapEditor
};

enum class SettingsTab {
    General,
    Audio,
    Credits
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
    float aggroRadius = 16.0f;
    bool alive = true;
    bool aggroed = false;
    float hitFlashTimer = 0.0f;
    float windupFlashTimer = 0.0f;
    float windupTimer = 0.0f;
    bool windingUp = false;
    float attackCooldown = 0.0f;
    float roamRetargetTimer = 0.0f;
    float scorchDpsAccum = 0.0f;
    float burnDpsAccum = 0.0f;
    ImpAiState ai = ImpAiState::Roam;
    std::vector<float> burnStacks{};
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
    bool appliesBurn = false;
};

struct ScorchZone {
    nu::Vec3 position{};
    float radius = 1.1f;
    float remaining = 3.0f;
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
    float dashCooldown = 0.0f;
    float dashCooldownDuration = 6.0f;
    float barrierCooldown = 0.0f;
    float barrierCooldownDuration = 16.0f;
    float barrierTimer = 0.0f;
    float barrierHp = 0.0f;
    float ultCooldown = 0.0f;
    float ultCooldownDuration = 45.0f;
    float ultTimer = 0.0f;
    float dashSpeedTimer = 0.0f;

    float damageFlash = 0.0f;
};

struct SkillTreeState {
    bool minorDashSteer = false;
    bool minorBarrierReload = false;
    bool majorDashSpeed = false;
    bool majorBarrierNova = false;
};

struct GameSettings {
    float mouseSensitivity = 1.0f;
    bool showDamageNumbers = true;
    float masterVolume = 1.0f;
    float guiScale = 1.5f;
};

struct UiIcon {
    nu::AllocatedImage image{};
    VkDescriptorSet descriptor = VK_NULL_HANDLE;
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
    void applyMapCollisions();
    void tryLoadUrns();
    void setupHotReload();
    void loadAbilityIcons();
    void destroyAbilityIcons();
    void applySettings();
    void openSettings(AppScreen returnScreen);
    void closeSettings();
    void enterMainMenu();
    void enterMapEditor();
    void leaveMapEditor(bool save);
    void startPlay();
    void pauseGame();
    void resumeGame();
    void killPlayer();
    void resetEncounter();
    void syncImpsFromMap();
    void killImp(ImpEnemy& imp);
    void pickImpRoamTarget(ImpEnemy& imp);
    void tryFireProjectile();
    void tryMeleeAttack();
    void tryDash();
    void tryBarrier();
    void tryUltimate();
    void tryReload();
    void onBarrierExpired();
    void triggerBarrierNova();
    void beginImpWindup(ImpEnemy& imp);
    void triggerHitFeel(bool heavy);
    void updateHitFeel(float dt);
    void updatePlayerMoveSpeed();
    void toggleSkillTree();
    void closeSkillTree();
    void tryUnlockSkill(int nodeId);
    void updateCombat(float dt);
    void updateImpAi(float dt);
    void updateImpAiOne(ImpEnemy& imp, float dt);
    void updateProjectiles(float dt);
    void updateZonesAndBurns(float dt);
    void updateVfx(float dt);
    void updateDamageNumbers(float dt);
    void updateNextLevelTrigger();
    void updateMapEditor(float dt);
    void damageImp(ImpEnemy& imp, float amount, const nu::Vec3& hitPos, bool applyBurn = false);
    void damagePlayer(float amount, const nu::Vec3& hitPos);
    void spawnImpSpit(ImpEnemy& imp);
    void spawnDamageNumber(const nu::Vec3& worldPos, float amount);
    void drawMainMenu();
    void drawPauseMenu();
    void drawDeathMenu();
    void drawSettingsMenu();
    void drawMapEditorUi();
    void drawGameplayHud();
    void drawFpsCounter();
    void drawPlayerHpBar();
    void drawAmmoPips();
    void drawAbilitySlots();
    void drawCrosshair();
    void drawImpHpBars();
    void drawDamageNumbers();
    void drawDamageVignette();
    void drawCurrencyPanel();
    void drawSkillTreeButton();
    void drawSkillTreePanel();
    void drawBarrierAura(nu::RenderScene& scene);
    void drawScorchZones(nu::RenderScene& scene);
    void drawMapObjects(nu::RenderScene& scene, bool editorView);
    void drawAggroRadii(nu::RenderScene& scene);

    [[nodiscard]] bool isSimulating() const;
    [[nodiscard]] bool barrierActive() const;
    [[nodiscard]] bool ultActive() const;
    [[nodiscard]] bool playerInInkPool();
    [[nodiscard]] float currentFireInterval() const;
    [[nodiscard]] float currentReloadDuration() const;
    [[nodiscard]] float currentFireDamage() const;
    [[nodiscard]] float gui() const;
    [[nodiscard]] float impMoveMultiplier(const ImpEnemy& imp) const;
    [[nodiscard]] nu::Vec3 playerFlatForward();
    [[nodiscard]] nu::Vec3 playerFlatMoveDir();
    [[nodiscard]] std::filesystem::path arenaMapPath() const;
    [[nodiscard]] bool screenToGround(float screenX, float screenY, nu::Vec3& out);

    AppScreen screen_ = AppScreen::MainMenu;
    AppScreen settingsReturn_ = AppScreen::MainMenu;
    SettingsTab settingsTab_ = SettingsTab::General;
    GameSettings settings_{};
    bool skillTreeOpen_ = false;
    SkillTreeState skills_{};
    ArenaMap arenaMap_{};
    int editorSelection_ = -3; // -2 spawn, -1 nextLevel, >=0 object, -3 none
    bool editorDragging_ = false;
    MapObjectKind editorPlaceKind_ = MapObjectKind::Imp;

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

    std::array<UiIcon, 4> abilityIcons_{};

    std::vector<ImpEnemy> imps_{};
    PlayerCombat player_{};
    int nox_ = 0;
    int pureNox_ = 0;
    float hitstopTimer_ = 0.0f;
    float cameraKickPitch_ = 0.0f;
    std::vector<Projectile> projectiles_;
    std::vector<ScorchZone> scorchZones_;
    std::vector<TrailParticle> trailParticles_;
    std::vector<MeleeSlashVfx> meleeVfx_;
    std::vector<DamageNumber> damageNumbers_;
    std::mt19937 rng_{std::random_device{}()};
};

} // namespace nine
