#include "game_app.hpp"

#include "arena_greybox.hpp"

#include "nu/assets/texture_loader.hpp"
#include "nu/core/log.hpp"
#include "nu/core/paths.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace nine {
namespace {

constexpr float kFireballSpeed = 28.0f;
constexpr float kFireballLifetime = 3.0f;
constexpr float kImpRadius = 0.55f;
constexpr float kImpHalfHeight = 0.9f;
constexpr float kPlayerRadius = 0.45f;
constexpr float kPlayerHalfHeight = 0.95f;
constexpr float kHitFlashDuration = 0.18f;
constexpr float kImpRoamSpeed = 1.7f;
constexpr float kImpChaseSpeed = 4.6f;
constexpr float kImpRetreatSpeed = 2.1f;
constexpr float kImpDesiredRange = 7.5f;
constexpr float kImpAttackRange = 11.0f;
constexpr float kImpAttackInterval = 1.35f;
constexpr float kImpWindupDuration = 0.45f;
constexpr float kImpWindupFlashDuration = 0.45f;
constexpr float kImpSpitSpeed = 24.0f;
constexpr float kImpSpitGravity = 16.0f;
constexpr float kImpSpitDamage = 40.0f;
constexpr float kMeleeRange = 2.6f;
constexpr float kMeleeHalfAngleCos = 0.25f;
constexpr float kMeleeDamage = 100.0f;
constexpr float kMeleeKnockback = 3.2f;
constexpr float kDashDistance = 6.0f;
constexpr float kScorchDuration = 3.0f;
constexpr float kScorchDps = 10.0f;
constexpr float kScorchTickInterval = 0.5f;
constexpr float kBarrierDuration = 4.0f;
constexpr float kUltDuration = 10.0f;
constexpr float kUltFireInterval = 0.12f;
constexpr float kUltReloadDuration = 0.65f;
constexpr float kUltFireDamage = 80.0f;
constexpr float kBurnDuration = 2.0f;
constexpr float kBurnDps = 10.0f;
constexpr float kBurnTickInterval = 0.5f;
constexpr float kLowHpThreshold = 0.25f;
constexpr float kHitstopLight = 0.035f;
constexpr float kHitstopHeavy = 0.055f;
constexpr float kCameraKickLight = 0.55f;
constexpr float kCameraKickHeavy = 1.1f;
constexpr float kDashSpeedBuffDuration = 2.0f;
constexpr float kDashSpeedBuffMul = 1.5f;
constexpr float kBarrierNovaDamage = 50.0f;
constexpr float kBarrierNovaRadius = 4.0f;
constexpr float kBarrierNovaKnockback = 4.0f;
constexpr float kNextLevelRadius = 1.6f;
constexpr int kMinorSkillCost = 2;
constexpr int kMajorSkillCost = 5;

[[nodiscard]] bool projectWorldToScreen(const nu::Camera& camera, const nu::Vec3& world, float fbWidth,
                                        float fbHeight, nu::Vec2& outScreen) {
    const nu::Vec4 clip = camera.viewProjection() * nu::Vec4{world.x, world.y, world.z, 1.0f};
    if (clip.w <= 0.001f) {
        return false;
    }
    const nu::Vec3 ndc = nu::Vec3{clip.x, clip.y, clip.z} / clip.w;
    if (ndc.z < -1.0f || ndc.z > 1.0f) {
        return false;
    }
    outScreen.x = (ndc.x * 0.5f + 0.5f) * fbWidth;
    outScreen.y = (ndc.y * 0.5f + 0.5f) * fbHeight;
    return true;
}

[[nodiscard]] bool pointHitsCapsule(const nu::Vec3& point, const nu::Vec3& feet, float radius, float halfHeight,
                                    float projRadius) {
    const nu::Vec3 center = feet + nu::Vec3{0.0f, halfHeight, 0.0f};
    const nu::Vec3 delta = point - center;
    const float radii = radius + projRadius;
    const float dxz = std::sqrt(delta.x * delta.x + delta.z * delta.z);
    if (dxz > radii) {
        return false;
    }
    return std::abs(delta.y) <= (halfHeight + projRadius);
}

[[nodiscard]] float flatDistance(const nu::Vec3& a, const nu::Vec3& b) {
    const float dx = a.x - b.x;
    const float dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

[[nodiscard]] nu::Vec3 flatDir(const nu::Vec3& from, const nu::Vec3& to) {
    nu::Vec3 d{to.x - from.x, 0.0f, to.z - from.z};
    const float len = glm::length(d);
    if (len < 1.0e-4f) {
        return {0.0f, 0.0f, 1.0f};
    }
    return d / len;
}

[[nodiscard]] ImTextureRef texRef(VkDescriptorSet set) {
    return ImTextureRef{static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(set))};
}

void drawScaledText(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* text, float scale) {
    if (scale <= 1.001f) {
        dl->AddText(pos, col, text);
        return;
    }
    dl->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale, pos, col, text);
}

[[nodiscard]] ImVec2 scaledTextSize(const char* text, float scale) {
    const ImVec2 size = ImGui::CalcTextSize(text);
    return ImVec2{size.x * scale, size.y * scale};
}

} // namespace

GameApp::GameApp() : nu::Application(nu::ApplicationDesc{"Nine Under", 1600, 900, false}) {}

void GameApp::onInit() {
    NU_GAME_INFO("Game init — Arena I (The Grey Threshold) v0.6");
    arenaMap_ = makeDefaultArenaMap();
    const auto mapPath = arenaMapPath();
    if (std::filesystem::exists(mapPath)) {
        loadArenaMap(mapPath, arenaMap_);
    } else {
        saveArenaMap(mapPath, arenaMap_);
    }
    buildArenaOne();
    tryLoadUrns();
    setupHotReload();
    loadAbilityIcons();
    applySettings();
    resetEncounter();
    enterMainMenu();
}

void GameApp::onShutdown() {
    destroyAbilityIcons();
    if (urnProp_) {
        urnProp_->destroy(renderer());
        urnProp_.reset();
    }
    if (!meshesReady_) {
        return;
    }
    renderer().meshes().destroy(floorMesh_);
    renderer().meshes().destroy(cubeMesh_);
    renderer().meshes().destroy(playerMesh_);
    renderer().meshes().destroy(wallMesh_);
    renderer().meshes().destroy(impMesh_);
    renderer().meshes().destroy(projectileMesh_);
    meshesReady_ = false;
}

std::filesystem::path GameApp::arenaMapPath() const {
    return nu::Paths::resolveData("maps/arena_i.json");
}

void GameApp::buildArenaOne() {
    auto& factory = renderer().meshes();
    floorMesh_ = factory.createPlane(140.0f, 10);
    cubeMesh_ = factory.createCube(1.0f);
    wallMesh_ = factory.createCube(1.0f);
    playerMesh_ = factory.createCapsule(0.4f, 0.5f, 10);
    impMesh_ = factory.createCapsule(0.35f, 0.45f, 10);
    projectileMesh_ = factory.createCube(1.0f);
    meshesReady_ = true;

    const ArenaGreybox arena =
        buildArenaOneGreybox(factory, physics(), floorMesh_, cubeMesh_, wallMesh_);
    staticDraws_ = arena.draws;
    applyMapCollisions();
    NU_GAME_INFO("Arena I graybox ready ({} draw items)", staticDraws_.size());
}

void GameApp::applyMapCollisions() {
    for (const MapObject& obj : arenaMap_.objects) {
        if (obj.kind == MapObjectKind::Ruin) {
            physics().addBox({1.0f, 0.5f, 1.0f}, obj.position, true);
        }
    }
}

void GameApp::tryLoadUrns() {
    const auto path = nu::Paths::resolveData("models/prop/prop_ink_urn.glb");
    if (!std::filesystem::exists(path)) {
        NU_GAME_INFO("No urn prop at {} (arena will run without urns)", path.string());
        return;
    }

    auto model = nu::ModelAsset::load(renderer(), path);
    if (!model) {
        NU_GAME_INFO("Failed to load urn {}", path.string());
        return;
    }

    urnTransforms_ = {
        glm::translate(nu::Mat4{1.0f}, nu::Vec3{-3.5f, 0.0f, 3.0f}),
        glm::translate(nu::Mat4{1.0f}, nu::Vec3{3.5f, 0.0f, 4.5f}),
    };

    for (const nu::Mat4& xf : urnTransforms_) {
        const nu::Vec3 pos = nu::Vec3{xf[3]};
        for (const nu::ModelCollision& col : model->collisions()) {
            if (col.type == nu::GltfCollisionType::Box) {
                physics().addBox(col.halfExtents, col.translation + pos, true);
            }
        }
        if (model->collisions().empty()) {
            physics().addBox({0.35f, 0.45f, 0.35f}, pos + nu::Vec3{0.0f, 0.45f, 0.0f}, true);
        }
    }

    urnProp_ = std::move(*model);
    NU_GAME_INFO("Loaded prop_ink_urn ({} instances)", urnTransforms_.size());
}

void GameApp::setupHotReload() {
    const auto modelsDir = nu::Paths::resolveData("models");
    hotReload_.watchDirectory(modelsDir, ".glb", [this](const std::filesystem::path& path) {
        if (path.filename() == "prop_ink_urn.glb") {
            renderer().waitIdle();
            if (urnProp_) {
                urnProp_->destroy(renderer());
                urnProp_.reset();
            }
            tryLoadUrns();
        }
    });
}

void GameApp::loadAbilityIcons() {
    destroyAbilityIcons();
    const char* files[4] = {"ui/icons/icon_1.png", "ui/icons/icon_2.png", "ui/icons/icon_3.png",
                            "ui/icons/icon_ulti.png"};
    for (int i = 0; i < 4; ++i) {
        const auto path = nu::Paths::resolveData(files[i]);
        auto cpu = nu::loadTextureCpu(path);
        if (!cpu) {
            NU_GAME_INFO("Missing ability icon {}", path.string());
            continue;
        }
        abilityIcons_[static_cast<size_t>(i)].image = nu::uploadTexture(renderer().device(), *cpu, true);
        abilityIcons_[static_cast<size_t>(i)].descriptor = ImGui_ImplVulkan_AddTexture(
            abilityIcons_[static_cast<size_t>(i)].image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
}

void GameApp::destroyAbilityIcons() {
    for (UiIcon& icon : abilityIcons_) {
        if (icon.descriptor) {
            ImGui_ImplVulkan_RemoveTexture(icon.descriptor);
            icon.descriptor = VK_NULL_HANDLE;
        }
        renderer().device().destroyImage(icon.image);
    }
}

void GameApp::applySettings() {
    input().setLookSensitivity(settings_.mouseSensitivity);
    audio().setMasterVolume(settings_.masterVolume);
    ImGui::GetIO().FontGlobalScale = gui();
}

bool GameApp::isSimulating() const {
    return screen_ == AppScreen::Playing && !skillTreeOpen_;
}

bool GameApp::barrierActive() const {
    return player_.barrierTimer > 0.0f && player_.barrierHp > 0.0f;
}

bool GameApp::ultActive() const {
    return player_.ultTimer > 0.0f;
}

float GameApp::currentFireInterval() const {
    return ultActive() ? kUltFireInterval : player_.fireInterval;
}

float GameApp::currentReloadDuration() const {
    return ultActive() ? kUltReloadDuration : player_.reloadDuration;
}

float GameApp::currentFireDamage() const {
    return ultActive() ? kUltFireDamage : 60.0f;
}

float GameApp::gui() const {
    return std::clamp(settings_.guiScale, 0.75f, 2.5f);
}

bool GameApp::playerInInkPool() {
    const nu::Vec3 p = character().position();
    for (const MapObject& obj : arenaMap_.objects) {
        if (obj.kind != MapObjectKind::InkPool) {
            continue;
        }
        if (flatDistance(p, obj.position) <= obj.radius * 1.6f) {
            return true;
        }
    }
    return false;
}

float GameApp::impMoveMultiplier(const ImpEnemy& imp) const {
    for (const ScorchZone& z : scorchZones_) {
        if (flatDistance(imp.position, z.position) <= z.radius) {
            return 0.5f;
        }
    }
    return 1.0f;
}

nu::Vec3 GameApp::playerFlatForward() {
    nu::Vec3 f = camera().forward();
    f.y = 0.0f;
    const float len = glm::length(f);
    if (len < 1.0e-4f) {
        return {0.0f, 0.0f, 1.0f};
    }
    return f / len;
}

nu::Vec3 GameApp::playerFlatMoveDir() {
    const nu::Vec2 move = input().moveAxis();
    if (glm::length(move) < 0.15f) {
        return playerFlatForward();
    }
    const nu::Vec3 forward = playerFlatForward();
    const nu::Vec3 right = glm::normalize(glm::cross(forward, nu::Vec3{0.0f, 1.0f, 0.0f}));
    nu::Vec3 dir = right * move.x + forward * move.y;
    const float len = glm::length(dir);
    if (len < 1.0e-4f) {
        return forward;
    }
    return dir / len;
}

void GameApp::enterMainMenu() {
    closeSkillTree();
    screen_ = AppScreen::MainMenu;
    gameplayActive_ = false;
    freeCameraMode_ = false;
    camera().setMode(nu::CameraMode::ThirdPerson);
    input().setRelativeMouse(false);
}

void GameApp::enterMapEditor() {
    closeSkillTree();
    saveArenaMap(arenaMapPath(), arenaMap_);
    screen_ = AppScreen::MapEditor;
    gameplayActive_ = true;
    freeCameraMode_ = true;
    camera().setMode(nu::CameraMode::Free);
    camera().setPosition(nu::Vec3{0.0f, 18.0f, 28.0f});
    camera().yaw = -90.0f;
    camera().pitch = -35.0f;
    camera().update();
    input().setRelativeMouse(false);
    editorSelection_ = -3;
    editorDragging_ = false;
}

void GameApp::leaveMapEditor(bool save) {
    if (save) {
        saveArenaMap(arenaMapPath(), arenaMap_);
    }
    freeCameraMode_ = false;
    camera().setMode(nu::CameraMode::ThirdPerson);
    enterMainMenu();
}

void GameApp::openSettings(AppScreen returnScreen) {
    closeSkillTree();
    settingsReturn_ = returnScreen;
    settingsTab_ = SettingsTab::General;
    screen_ = AppScreen::Settings;
    gameplayActive_ = false;
    input().setRelativeMouse(false);
}

void GameApp::closeSettings() {
    applySettings();
    if (settingsReturn_ == AppScreen::Paused) {
        screen_ = AppScreen::Paused;
        return;
    }
    enterMainMenu();
}

void GameApp::startPlay() {
    if (std::filesystem::exists(arenaMapPath())) {
        loadArenaMap(arenaMapPath(), arenaMap_);
    }
    resetEncounter();
    screen_ = AppScreen::Playing;
    gameplayActive_ = true;
    freeCameraMode_ = false;
    camera().setMode(nu::CameraMode::ThirdPerson);
    skillTreeOpen_ = false;
    input().setRelativeMouse(true);
}

void GameApp::pauseGame() {
    if (screen_ != AppScreen::Playing) {
        return;
    }
    closeSkillTree();
    screen_ = AppScreen::Paused;
    gameplayActive_ = false;
    input().setRelativeMouse(false);
}

void GameApp::resumeGame() {
    if (screen_ != AppScreen::Paused) {
        return;
    }
    screen_ = AppScreen::Playing;
    gameplayActive_ = true;
    input().setRelativeMouse(true);
}

void GameApp::syncImpsFromMap() {
    imps_.clear();
    for (const MapObject& obj : arenaMap_.objects) {
        if (obj.kind != MapObjectKind::Imp) {
            continue;
        }
        ImpEnemy imp{};
        imp.spawnPosition = obj.position;
        imp.position = obj.position;
        imp.roamTarget = obj.position;
        imp.aggroRadius = obj.aggroRadius > 0.5f ? obj.aggroRadius : 16.0f;
        imp.hp = imp.maxHp;
        imp.attackCooldown = 0.6f;
        pickImpRoamTarget(imp);
        imps_.push_back(imp);
    }
}

void GameApp::resetEncounter() {
    player_ = PlayerCombat{};
    skills_ = SkillTreeState{};
    projectiles_.clear();
    scorchZones_.clear();
    trailParticles_.clear();
    meleeVfx_.clear();
    damageNumbers_.clear();
    nox_ = 0;
    pureNox_ = 0;
    hitstopTimer_ = 0.0f;
    cameraKickPitch_ = 0.0f;
    skillTreeOpen_ = false;
    character().setPosition(arenaMap_.spawnPoint);
    camera().yaw = -40.0f;
    camera().pitch = -20.0f;
    camera().setTarget(character().position());
    camera().update();
    syncImpsFromMap();
}

void GameApp::pickImpRoamTarget(ImpEnemy& imp) {
    std::uniform_real_distribution<float> dist(-6.0f, 6.0f);
    imp.roamTarget = imp.spawnPosition + nu::Vec3{dist(rng_), 0.0f, dist(rng_)};
    imp.roamRetargetTimer = std::uniform_real_distribution<float>(2.5f, 4.5f)(rng_);
}

void GameApp::killImp(ImpEnemy& imp) {
    if (!imp.alive) {
        return;
    }
    imp.hp = 0.0f;
    imp.alive = false;
    imp.aggroed = false;
    imp.windingUp = false;
    imp.windupTimer = 0.0f;
    imp.windupFlashTimer = 0.0f;
    imp.burnStacks.clear();
    ++nox_;
    ++pureNox_;

    std::uniform_real_distribution<float> pitchDist(0.88f, 1.12f);
    audio().playSound(nu::Paths::resolveData("audio/sfx/imp_death.wav"), pitchDist(rng_));
    NU_GAME_INFO("Imp died (+1 Nox, +1 Pure Nox)");
}

void GameApp::killPlayer() {
    if (screen_ != AppScreen::Playing && screen_ != AppScreen::Paused) {
        return;
    }
    closeSkillTree();
    player_.hp = 0.0f;
    screen_ = AppScreen::Death;
    gameplayActive_ = false;
    input().setRelativeMouse(false);
    projectiles_.clear();
    NU_GAME_INFO("Player died");
}

void GameApp::onEscapePressed() {
    if (skillTreeOpen_) {
        closeSkillTree();
        return;
    }
    if (screen_ == AppScreen::MapEditor) {
        leaveMapEditor(true);
        return;
    }
    if (screen_ == AppScreen::Settings) {
        closeSettings();
        return;
    }
    if (screen_ == AppScreen::Playing) {
        pauseGame();
        return;
    }
    if (screen_ == AppScreen::Paused) {
        resumeGame();
        return;
    }
    if (screen_ == AppScreen::Death) {
        enterMainMenu();
        return;
    }
    requestQuit();
}

void GameApp::toggleSkillTree() {
    if (screen_ != AppScreen::Playing) {
        return;
    }
    skillTreeOpen_ = !skillTreeOpen_;
    if (skillTreeOpen_) {
        gameplayActive_ = false;
        input().setRelativeMouse(false);
    } else {
        gameplayActive_ = true;
        input().setRelativeMouse(true);
    }
}

void GameApp::closeSkillTree() {
    if (!skillTreeOpen_) {
        return;
    }
    skillTreeOpen_ = false;
    if (screen_ == AppScreen::Playing) {
        gameplayActive_ = true;
        input().setRelativeMouse(true);
    }
}

void GameApp::tryUnlockSkill(int nodeId) {
    switch (nodeId) {
    case 1:
        if (!skills_.minorDashSteer && nox_ >= kMinorSkillCost) {
            nox_ -= kMinorSkillCost;
            skills_.minorDashSteer = true;
        }
        break;
    case 2:
        if (!skills_.minorBarrierReload && nox_ >= kMinorSkillCost) {
            nox_ -= kMinorSkillCost;
            skills_.minorBarrierReload = true;
        }
        break;
    case 3:
        if (skills_.minorDashSteer && !skills_.majorDashSpeed && pureNox_ >= kMajorSkillCost) {
            pureNox_ -= kMajorSkillCost;
            skills_.majorDashSpeed = true;
        }
        break;
    case 4:
        if (skills_.minorBarrierReload && !skills_.majorBarrierNova && pureNox_ >= kMajorSkillCost) {
            pureNox_ -= kMajorSkillCost;
            skills_.majorBarrierNova = true;
        }
        break;
    default:
        break;
    }
}

void GameApp::spawnDamageNumber(const nu::Vec3& worldPos, float amount) {
    if (!settings_.showDamageNumbers) {
        return;
    }
    DamageNumber dn;
    dn.worldPos = worldPos;
    dn.amount = amount;
    damageNumbers_.push_back(dn);
}

void GameApp::triggerHitFeel(bool heavy) {
    hitstopTimer_ = std::max(hitstopTimer_, heavy ? kHitstopHeavy : kHitstopLight);
    const float kick = heavy ? kCameraKickHeavy : kCameraKickLight;
    camera().pitch = std::clamp(camera().pitch - kick, -89.0f, 89.0f);
    cameraKickPitch_ = kick;
    camera().update();
}

void GameApp::updateHitFeel(float dt) {
    if (hitstopTimer_ > 0.0f) {
        hitstopTimer_ = std::max(0.0f, hitstopTimer_ - dt);
    }
    if (cameraKickPitch_ > 0.001f) {
        const float recover = std::min(cameraKickPitch_, cameraKickPitch_ * dt * 14.0f + dt * 2.0f);
        camera().pitch = std::clamp(camera().pitch + recover, -89.0f, 89.0f);
        cameraKickPitch_ = std::max(0.0f, cameraKickPitch_ - recover);
        camera().update();
    }
}

void GameApp::updatePlayerMoveSpeed() {
    float mul = 1.0f;
    if (playerInInkPool()) {
        mul *= 0.5f;
    }
    if (player_.dashSpeedTimer > 0.0f) {
        mul *= kDashSpeedBuffMul;
    }
    character().setMoveSpeedMultiplier(mul);
}

void GameApp::damageImp(ImpEnemy& imp, float amount, const nu::Vec3& hitPos, bool applyBurn) {
    if (!imp.alive) {
        return;
    }
    imp.hp = std::max(0.0f, imp.hp - amount);
    imp.hitFlashTimer = kHitFlashDuration;
    spawnDamageNumber(hitPos + nu::Vec3{0.0f, 0.35f, 0.0f}, amount);

    if (applyBurn) {
        imp.burnStacks.push_back(kBurnDuration);
    }

    // Damage always aggroes, even outside aggro radius.
    if (!imp.aggroed) {
        imp.aggroed = true;
        imp.ai = ImpAiState::Aggro;
    }

    if (imp.hp <= 0.0f) {
        killImp(imp);
    }
}

void GameApp::damagePlayer(float amount, const nu::Vec3& hitPos) {
    if (!isSimulating()) {
        return;
    }
    player_.hp = std::max(0.0f, player_.hp - amount);
    player_.damageFlash = std::max(player_.damageFlash, 0.45f);
    spawnDamageNumber(hitPos + nu::Vec3{0.0f, 0.5f, 0.0f}, amount);

    if (player_.hp <= 0.0f) {
        killPlayer();
    }
}

void GameApp::tryFireProjectile() {
    if (player_.fireCooldown > 0.0f || player_.reloading) {
        return;
    }

    nu::Vec3 dir = camera().forward();
    if (glm::dot(dir, dir) < 1.0e-6f) {
        return;
    }
    dir = glm::normalize(dir);

    Projectile proj;
    proj.position = character().position() + nu::Vec3{0.0f, 1.1f, 0.0f} + dir * 0.6f;
    proj.velocity = dir * kFireballSpeed;
    proj.lifetime = kFireballLifetime;
    proj.damage = currentFireDamage();
    proj.owner = ProjectileOwner::Player;
    proj.appliesBurn = ultActive();
    projectiles_.push_back(proj);

    player_.fireCooldown = currentFireInterval();
    ++player_.shotsInClip;
    if (player_.shotsInClip >= player_.clipSize) {
        player_.reloading = true;
        player_.reloadTimer = currentReloadDuration();
        player_.shotsInClip = 0;
    }
}

void GameApp::tryMeleeAttack() {
    if (player_.meleeCooldown > 0.0f) {
        return;
    }
    player_.meleeCooldown = player_.meleeCooldownDuration;

    const nu::Vec3 origin = character().position() + nu::Vec3{0.0f, 1.0f, 0.0f};
    const nu::Vec3 forward = playerFlatForward();

    MeleeSlashVfx slash;
    slash.position = origin + forward * 1.1f;
    slash.forward = forward;
    meleeVfx_.push_back(slash);

    for (ImpEnemy& imp : imps_) {
        if (!imp.alive) {
            continue;
        }
        const float dist = flatDistance(origin, imp.position);
        if (dist > kMeleeRange || dist < 1.0e-3f) {
            continue;
        }
        const nu::Vec3 toFlat = flatDir(origin, imp.position);
        if (glm::dot(forward, toFlat) < kMeleeHalfAngleCos) {
            continue;
        }
        damageImp(imp, kMeleeDamage, imp.position + nu::Vec3{0.0f, 1.2f, 0.0f}, ultActive());
        triggerHitFeel(true);
        if (imp.alive) {
            imp.position += toFlat * kMeleeKnockback;
        }
        audio().playSound(nu::Paths::resolveData("audio/sfx/fireball_hit.wav"),
                          std::uniform_real_distribution<float>(0.75f, 0.9f)(rng_));
    }
}

void GameApp::tryDash() {
    if (player_.dashCooldown > 0.0f) {
        return;
    }
    player_.dashCooldown = player_.dashCooldownDuration;

    const nu::Vec3 dir = skills_.minorDashSteer ? playerFlatMoveDir() : playerFlatForward();
    const nu::Vec3 start = character().position();
    const nu::Vec3 end = start + dir * kDashDistance;
    character().setPosition(end);

    if (skills_.majorDashSpeed) {
        player_.dashSpeedTimer = kDashSpeedBuffDuration;
    }

    constexpr int kSegments = 8;
    for (int i = 0; i <= kSegments; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSegments);
        ScorchZone zone;
        zone.position = start + (end - start) * t;
        zone.position.y = 0.02f;
        zone.remaining = kScorchDuration;
        scorchZones_.push_back(zone);
    }
}

void GameApp::tryBarrier() {
    if (player_.barrierCooldown > 0.0f || barrierActive()) {
        return;
    }
    player_.barrierCooldown = player_.barrierCooldownDuration;
    player_.barrierTimer = kBarrierDuration;
    player_.barrierHp = player_.maxHp * 0.5f;
}

void GameApp::tryUltimate() {
    if (player_.ultCooldown > 0.0f || ultActive()) {
        return;
    }
    player_.ultCooldown = player_.ultCooldownDuration;
    player_.ultTimer = kUltDuration;
    player_.reloading = false;
    player_.reloadTimer = 0.0f;
    player_.shotsInClip = 0;
}

void GameApp::tryReload() {
    if (player_.reloading || player_.shotsInClip <= 0) {
        return;
    }
    player_.reloading = true;
    player_.reloadTimer = currentReloadDuration();
    player_.shotsInClip = 0;
}

void GameApp::onBarrierExpired() {
    if (skills_.minorBarrierReload) {
        player_.reloading = false;
        player_.reloadTimer = 0.0f;
        player_.shotsInClip = 0;
    }
    if (skills_.majorBarrierNova) {
        triggerBarrierNova();
    }
}

void GameApp::triggerBarrierNova() {
    const nu::Vec3 origin = character().position();
    for (ImpEnemy& imp : imps_) {
        if (!imp.alive) {
            continue;
        }
        if (flatDistance(origin, imp.position) > kBarrierNovaRadius) {
            continue;
        }
        const nu::Vec3 push = flatDir(origin, imp.position);
        damageImp(imp, kBarrierNovaDamage, imp.position + nu::Vec3{0.0f, 1.0f, 0.0f}, false);
        if (imp.alive) {
            imp.position += push * kBarrierNovaKnockback;
        }
    }
    triggerHitFeel(true);
    audio().playSound(nu::Paths::resolveData("audio/sfx/fireball_hit.wav"),
                      std::uniform_real_distribution<float>(0.7f, 0.85f)(rng_));
}

void GameApp::beginImpWindup(ImpEnemy& imp) {
    if (!imp.alive || imp.windingUp) {
        return;
    }
    imp.windingUp = true;
    imp.windupTimer = kImpWindupDuration;
    imp.windupFlashTimer = kImpWindupFlashDuration;
    std::uniform_real_distribution<float> pitchDist(0.90f, 1.15f);
    audio().playSound(nu::Paths::resolveData("audio/sfx/imp_windup.wav"), pitchDist(rng_));
}

void GameApp::spawnImpSpit(ImpEnemy& imp) {
    const nu::Vec3 start = imp.position + nu::Vec3{0.0f, 1.25f, 0.0f};
    const nu::Vec3 target = character().position() + nu::Vec3{0.0f, 1.05f, 0.0f};
    const nu::Vec3 delta = target - start;
    const float horiz = std::sqrt(delta.x * delta.x + delta.z * delta.z);
    float t = std::max(0.25f, horiz / std::max(1.0f, kImpSpitSpeed * 0.9f));
    t = std::min(t, 1.25f);

    Projectile spit;
    spit.position = start;
    spit.velocity = {delta.x / t, (delta.y + 0.5f * kImpSpitGravity * t * t) / t, delta.z / t};
    spit.lifetime = t + 0.6f;
    spit.radius = 0.22f;
    spit.damage = kImpSpitDamage;
    spit.gravity = kImpSpitGravity;
    spit.owner = ProjectileOwner::Imp;
    projectiles_.push_back(spit);
}

void GameApp::updateImpAiOne(ImpEnemy& imp, float dt) {
    if (!imp.alive) {
        return;
    }

    if (imp.hitFlashTimer > 0.0f) {
        imp.hitFlashTimer = std::max(0.0f, imp.hitFlashTimer - dt);
    }
    if (imp.windupFlashTimer > 0.0f) {
        imp.windupFlashTimer = std::max(0.0f, imp.windupFlashTimer - dt);
    }
    if (imp.attackCooldown > 0.0f) {
        imp.attackCooldown = std::max(0.0f, imp.attackCooldown - dt);
    }

    const nu::Vec3 playerPos = character().position();
    const float dist = flatDistance(imp.position, playerPos);
    const float moveMul = impMoveMultiplier(imp);

    if (!imp.aggroed && dist <= imp.aggroRadius) {
        imp.aggroed = true;
        imp.ai = ImpAiState::Aggro;
    }

    if (imp.windingUp) {
        imp.windupTimer -= dt;
        if (imp.windupTimer <= 0.0f) {
            imp.windingUp = false;
            imp.windupTimer = 0.0f;
            spawnImpSpit(imp);
            imp.attackCooldown = kImpAttackInterval;
        }
        return;
    }

    if (imp.ai == ImpAiState::Roam) {
        imp.roamRetargetTimer -= dt;
        if (imp.roamRetargetTimer <= 0.0f || flatDistance(imp.position, imp.roamTarget) < 0.4f) {
            pickImpRoamTarget(imp);
        }
        imp.position += flatDir(imp.position, imp.roamTarget) * (kImpRoamSpeed * moveMul * dt);
        return;
    }

    const float rangeError = dist - kImpDesiredRange;
    if (std::abs(rangeError) > 0.35f) {
        const nu::Vec3 toward = flatDir(imp.position, playerPos);
        const nu::Vec3 moveDir = rangeError > 0.0f ? toward : -toward;
        const float speed = rangeError > 0.0f ? kImpChaseSpeed : kImpRetreatSpeed;
        imp.position += moveDir * (speed * moveMul * dt);
    }

    if (dist <= kImpAttackRange && imp.attackCooldown <= 0.0f) {
        beginImpWindup(imp);
    }
}

void GameApp::updateImpAi(float dt) {
    for (ImpEnemy& imp : imps_) {
        updateImpAiOne(imp, dt);
    }
}

void GameApp::updateCombat(float dt) {
    if (player_.fireCooldown > 0.0f) {
        player_.fireCooldown = std::max(0.0f, player_.fireCooldown - dt);
    }
    if (player_.meleeCooldown > 0.0f) {
        player_.meleeCooldown = std::max(0.0f, player_.meleeCooldown - dt);
    }
    if (player_.dashCooldown > 0.0f) {
        player_.dashCooldown = std::max(0.0f, player_.dashCooldown - dt);
    }
    if (player_.barrierCooldown > 0.0f) {
        player_.barrierCooldown = std::max(0.0f, player_.barrierCooldown - dt);
    }
    if (player_.ultCooldown > 0.0f) {
        player_.ultCooldown = std::max(0.0f, player_.ultCooldown - dt);
    }
    if (player_.dashSpeedTimer > 0.0f) {
        player_.dashSpeedTimer = std::max(0.0f, player_.dashSpeedTimer - dt);
    }
    if (player_.barrierTimer > 0.0f) {
        const bool wasActive = barrierActive();
        player_.barrierTimer = std::max(0.0f, player_.barrierTimer - dt);
        if (wasActive && (player_.barrierTimer <= 0.0f || player_.barrierHp <= 0.0f)) {
            player_.barrierTimer = 0.0f;
            player_.barrierHp = 0.0f;
            onBarrierExpired();
        }
    }
    if (player_.ultTimer > 0.0f) {
        player_.ultTimer = std::max(0.0f, player_.ultTimer - dt);
    }
    if (player_.damageFlash > 0.0f) {
        player_.damageFlash = std::max(0.0f, player_.damageFlash - dt);
    }

    if (player_.reloading) {
        player_.reloadTimer -= dt;
        if (player_.reloadTimer <= 0.0f) {
            player_.reloading = false;
            player_.reloadTimer = 0.0f;
        }
    }

    if (freeCameraMode_) {
        return;
    }

    if (input().mouseDown(nu::MouseButton::Left)) {
        tryFireProjectile();
    }
    if (input().keyPressed(nu::Key::Q)) {
        tryMeleeAttack();
    }
    if (input().keyPressed(nu::Key::LeftShift)) {
        tryDash();
    }
    if (input().keyPressed(nu::Key::E)) {
        tryBarrier();
    }
    if (input().keyPressed(nu::Key::Space)) {
        tryUltimate();
    }
    if (input().keyPressed(nu::Key::R)) {
        tryReload();
    }
}

void GameApp::updateProjectiles(float dt) {
    for (Projectile& proj : projectiles_) {
        if (proj.lifetime <= 0.0f) {
            continue;
        }

        if (proj.gravity > 0.0f) {
            proj.velocity.y -= proj.gravity * dt;
        }
        proj.position += proj.velocity * dt;
        proj.lifetime -= dt;

        if (proj.owner == ProjectileOwner::Player) {
            proj.trailEmitTimer -= dt;
            if (proj.trailEmitTimer <= 0.0f) {
                TrailParticle p;
                p.position = proj.position;
                p.scale = 0.12f + 0.06f * std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_);
                trailParticles_.push_back(p);
                proj.trailEmitTimer = 0.028f;
            }
            for (ImpEnemy& imp : imps_) {
                if (!imp.alive) {
                    continue;
                }
                if (pointHitsCapsule(proj.position, imp.position, kImpRadius, kImpHalfHeight, proj.radius)) {
                    damageImp(imp, proj.damage, proj.position, proj.appliesBurn);
                    triggerHitFeel(proj.appliesBurn);
                    audio().playSound(nu::Paths::resolveData("audio/sfx/fireball_hit.wav"),
                                      std::uniform_real_distribution<float>(0.92f, 1.08f)(rng_));
                    proj.lifetime = 0.0f;
                    break;
                }
            }
        } else if (pointHitsCapsule(proj.position, character().position(), kPlayerRadius, kPlayerHalfHeight,
                                    proj.radius)) {
            if (barrierActive()) {
                player_.barrierHp -= proj.damage;
                if (player_.barrierHp <= 0.0f) {
                    player_.barrierHp = 0.0f;
                    player_.barrierTimer = 0.0f;
                    onBarrierExpired();
                }
            } else {
                damagePlayer(proj.damage, proj.position);
            }
            proj.lifetime = 0.0f;
        }

        if (proj.position.y < -1.0f) {
            proj.lifetime = 0.0f;
        }
    }

    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(),
                                      [](const Projectile& p) { return p.lifetime <= 0.0f; }),
                       projectiles_.end());
}

void GameApp::updateZonesAndBurns(float dt) {
    for (ScorchZone& z : scorchZones_) {
        z.remaining -= dt;
    }
    scorchZones_.erase(std::remove_if(scorchZones_.begin(), scorchZones_.end(),
                                      [](const ScorchZone& z) { return z.remaining <= 0.0f; }),
                       scorchZones_.end());

    for (ImpEnemy& imp : imps_) {
        if (!imp.alive) {
            continue;
        }
        if (impMoveMultiplier(imp) < 1.0f) {
            imp.scorchDpsAccum += dt;
            if (imp.scorchDpsAccum >= kScorchTickInterval) {
                const float ticks = std::floor(imp.scorchDpsAccum / kScorchTickInterval);
                imp.scorchDpsAccum -= ticks * kScorchTickInterval;
                damageImp(imp, ticks * kScorchDps * kScorchTickInterval, imp.position + nu::Vec3{0.0f, 1.0f, 0.0f});
            }
        } else {
            imp.scorchDpsAccum = 0.0f;
        }

        if (!imp.burnStacks.empty()) {
            float burnDamage = 0.0f;
            for (float& stack : imp.burnStacks) {
                burnDamage += kBurnDps * dt;
                stack -= dt;
            }
            imp.burnStacks.erase(std::remove_if(imp.burnStacks.begin(), imp.burnStacks.end(),
                                                [](float t) { return t <= 0.0f; }),
                                 imp.burnStacks.end());
            imp.burnDpsAccum += burnDamage;
            const float tickDamage = kBurnDps * kBurnTickInterval;
            if (imp.burnDpsAccum >= tickDamage) {
                const float ticks = std::floor(imp.burnDpsAccum / tickDamage);
                imp.burnDpsAccum -= ticks * tickDamage;
                damageImp(imp, ticks * tickDamage, imp.position + nu::Vec3{0.0f, 1.4f, 0.0f});
            }
        } else {
            imp.burnDpsAccum = 0.0f;
        }
    }
}

void GameApp::updateVfx(float dt) {
    for (TrailParticle& p : trailParticles_) {
        p.age += dt;
        p.position.y += 0.35f * dt;
    }
    trailParticles_.erase(std::remove_if(trailParticles_.begin(), trailParticles_.end(),
                                         [](const TrailParticle& p) { return p.age >= p.lifetime; }),
                          trailParticles_.end());

    for (MeleeSlashVfx& s : meleeVfx_) {
        s.age += dt;
    }
    meleeVfx_.erase(std::remove_if(meleeVfx_.begin(), meleeVfx_.end(),
                                   [](const MeleeSlashVfx& s) { return s.age >= s.lifetime; }),
                    meleeVfx_.end());
}

void GameApp::updateDamageNumbers(float dt) {
    for (DamageNumber& dn : damageNumbers_) {
        dn.age += dt;
        dn.worldPos.y += 1.2f * dt;
    }
    damageNumbers_.erase(std::remove_if(damageNumbers_.begin(), damageNumbers_.end(),
                                        [](const DamageNumber& d) { return d.age >= d.lifetime; }),
                         damageNumbers_.end());
}

void GameApp::updateNextLevelTrigger() {
    if (flatDistance(character().position(), arenaMap_.nextLevelPoint) <= kNextLevelRadius) {
        NU_GAME_INFO("Reached next-level marker — returning to main menu");
        enterMainMenu();
    }
}

bool GameApp::screenToGround(float screenX, float screenY, nu::Vec3& out) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float ndcX = ((screenX - viewport->Pos.x) / viewport->Size.x) * 2.0f - 1.0f;
    const float ndcY = 1.0f - ((screenY - viewport->Pos.y) / viewport->Size.y) * 2.0f;
    const nu::Mat4 inv = glm::inverse(camera().viewProjection());
    auto unproject = [&](float z) {
        nu::Vec4 clip{ndcX, ndcY, z, 1.0f};
        nu::Vec4 world = inv * clip;
        if (std::abs(world.w) < 1.0e-5f) {
            return nu::Vec3{0.0f};
        }
        world /= world.w;
        return nu::Vec3{world.x, world.y, world.z};
    };
    const nu::Vec3 nearPt = unproject(-1.0f);
    const nu::Vec3 farPt = unproject(1.0f);
    const nu::Vec3 dir = farPt - nearPt;
    if (std::abs(dir.y) < 1.0e-5f) {
        return false;
    }
    const float t = -nearPt.y / dir.y;
    if (t < 0.0f || t > 1.0f) {
        return false;
    }
    out = nearPt + dir * t;
    return true;
}

void GameApp::updateMapEditor(float dt) {
    (void)dt;
    if (ImGui::GetIO().WantCaptureMouse) {
        editorDragging_ = false;
        return;
    }

    const nu::Vec2 mouse = input().mousePosition();
    nu::Vec3 ground{};
    const bool hitGround = screenToGround(mouse.x, mouse.y, ground);

    if (input().mousePressed(nu::MouseButton::Left) && hitGround) {
        float best = 2.2f;
        int bestSel = -3;
        if (flatDistance(ground, arenaMap_.spawnPoint) < best) {
            best = flatDistance(ground, arenaMap_.spawnPoint);
            bestSel = -2;
        }
        if (flatDistance(ground, arenaMap_.nextLevelPoint) < best) {
            best = flatDistance(ground, arenaMap_.nextLevelPoint);
            bestSel = -1;
        }
        for (int i = 0; i < static_cast<int>(arenaMap_.objects.size()); ++i) {
            const float d = flatDistance(ground, arenaMap_.objects[static_cast<size_t>(i)].position);
            if (d < best) {
                best = d;
                bestSel = i;
            }
        }
        editorSelection_ = bestSel;
        editorDragging_ = bestSel != -3;
    }

    if (!input().mouseDown(nu::MouseButton::Left)) {
        editorDragging_ = false;
    }

    if (editorDragging_ && hitGround) {
        ground.y = 0.0f;
        if (editorSelection_ == -2) {
            arenaMap_.spawnPoint = {ground.x, 2.0f, ground.z};
        } else if (editorSelection_ == -1) {
            arenaMap_.nextLevelPoint = {ground.x, 0.05f, ground.z};
        } else if (editorSelection_ >= 0 &&
                   editorSelection_ < static_cast<int>(arenaMap_.objects.size())) {
            MapObject& obj = arenaMap_.objects[static_cast<size_t>(editorSelection_)];
            if (obj.kind == MapObjectKind::Ruin) {
                ground.y = 0.5f;
            } else if (obj.kind == MapObjectKind::InkPool) {
                ground.y = 0.05f;
            }
            obj.position = ground;
        }
    }

}

void GameApp::onFixedUpdate(float dt) {
    updateHitFeel(dt);

    if (screen_ == AppScreen::Playing && input().consumeKeyPressed(nu::Key::Tab)) {
        toggleSkillTree();
    }

    if (screen_ == AppScreen::MapEditor) {
        gameplayActive_ = true;
        freeCameraMode_ = true;
        Application::onFixedUpdate(dt);
        updateMapEditor(dt);
        return;
    }

    if (!isSimulating()) {
        gameplayActive_ = false;
        Application::onFixedUpdate(dt);
        return;
    }

    gameplayActive_ = true;
    updatePlayerMoveSpeed();

    if (hitstopTimer_ > 0.0f) {
        Application::onFixedUpdate(dt);
        return;
    }

    Application::onFixedUpdate(dt);
    updateCombat(dt);
    updateImpAi(dt);
    updateProjectiles(dt);
    updateZonesAndBurns(dt);
    updateVfx(dt);
    updateDamageNumbers(dt);
    updateNextLevelTrigger();
}

void GameApp::onVariableUpdate(float dt) {
    Application::onVariableUpdate(dt);
    hotReload_.poll();
    (void)dt;
}

void GameApp::drawMainMenu() {
    const float s = gui();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2{viewport->WorkPos.x + 16.0f * s, viewport->WorkPos.y + 16.0f * s},
                            ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("##MapEditorEntry", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBackground);
    ImGui::SetWindowFontScale(s);
    if (ImGui::Button("Map Editor", ImVec2{160.0f * s, 36.0f * s})) {
        enterMapEditor();
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();

    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{360.0f * s, 280.0f * s}, ImGuiCond_Always);
    ImGui::Begin("##MainMenu", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(s);

    ImGui::Dummy(ImVec2{0.0f, 12.0f * s});
    const float titleWidth = ImGui::CalcTextSize("NINE UNDER").x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleWidth) * 0.5f);
    ImGui::TextUnformatted("NINE UNDER");
    ImGui::Dummy(ImVec2{0.0f, 4.0f * s});
    const float subWidth = ImGui::CalcTextSize("The Grey Threshold").x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - subWidth) * 0.5f);
    ImGui::TextDisabled("The Grey Threshold");
    ImGui::Dummy(ImVec2{0.0f, 20.0f * s});

    const float btnW = 220.0f * s;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Play", ImVec2{btnW, 40.0f * s})) {
        startPlay();
    }
    ImGui::Dummy(ImVec2{0.0f, 8.0f * s});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Settings", ImVec2{btnW, 36.0f * s})) {
        openSettings(AppScreen::MainMenu);
    }
    ImGui::Dummy(ImVec2{0.0f, 8.0f * s});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Quit", ImVec2{btnW, 36.0f * s})) {
        requestQuit();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
}

void GameApp::drawPauseMenu() {
    const float s = gui();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{360.0f * s, 280.0f * s}, ImGuiCond_Always);
    ImGui::Begin("##PauseMenu", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(s);

    ImGui::Dummy(ImVec2{0.0f, 16.0f * s});
    const float titleWidth = ImGui::CalcTextSize("PAUSED").x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleWidth) * 0.5f);
    ImGui::TextUnformatted("PAUSED");
    ImGui::Dummy(ImVec2{0.0f, 24.0f * s});

    const float btnW = 220.0f * s;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Resume", ImVec2{btnW, 40.0f * s})) {
        resumeGame();
    }
    ImGui::Dummy(ImVec2{0.0f, 8.0f * s});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Settings", ImVec2{btnW, 36.0f * s})) {
        openSettings(AppScreen::Paused);
    }
    ImGui::Dummy(ImVec2{0.0f, 8.0f * s});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Main Menu", ImVec2{btnW, 36.0f * s})) {
        enterMainMenu();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
}

void GameApp::drawDeathMenu() {
    const float s = gui();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{360.0f * s, 240.0f * s}, ImGuiCond_Always);
    ImGui::Begin("##DeathMenu", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(s);

    ImGui::Dummy(ImVec2{0.0f, 16.0f * s});
    const float titleWidth = ImGui::CalcTextSize("YOU DIED").x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleWidth) * 0.5f);
    ImGui::TextUnformatted("YOU DIED");
    ImGui::Dummy(ImVec2{0.0f, 28.0f * s});

    const float btnW = 220.0f * s;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Play Again", ImVec2{btnW, 40.0f * s})) {
        startPlay();
    }
    ImGui::Dummy(ImVec2{0.0f, 10.0f * s});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Main Menu", ImVec2{btnW, 40.0f * s})) {
        enterMainMenu();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
}

void GameApp::drawSettingsMenu() {
    const float s = gui();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{480.0f * s, 400.0f * s}, ImGuiCond_Always);
    ImGui::Begin("##Settings", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(s);

    ImGui::TextUnformatted("SETTINGS");
    ImGui::Separator();
    if (ImGui::BeginTabBar("##SettingsTabs")) {
        if (ImGui::BeginTabItem("General")) {
            settingsTab_ = SettingsTab::General;
            ImGui::Dummy(ImVec2{0.0f, 8.0f * s});
            ImGui::SliderFloat("Mouse sensitivity", &settings_.mouseSensitivity, 0.2f, 2.5f, "%.2f");
            if (ImGui::SliderFloat("GUI scale", &settings_.guiScale, 0.75f, 2.5f, "%.2f")) {
                ImGui::GetIO().FontGlobalScale = gui();
            }
            ImGui::Checkbox("Show floating damage numbers", &settings_.showDamageNumbers);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Audio")) {
            settingsTab_ = SettingsTab::Audio;
            ImGui::Dummy(ImVec2{0.0f, 8.0f * s});
            if (ImGui::SliderFloat("Volume", &settings_.masterVolume, 0.0f, 1.0f, "%.2f")) {
                audio().setMasterVolume(settings_.masterVolume);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Credits")) {
            settingsTab_ = SettingsTab::Credits;
            ImGui::Dummy(ImVec2{0.0f, 8.0f * s});
            ImGui::TextWrapped(
                "Audio assets courtesy of Zapsplat (www.zapsplat.com). Thank you for the SFX library.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Dummy(ImVec2{0.0f, 16.0f * s});
    if (ImGui::Button("Back", ImVec2{140.0f * s, 34.0f * s})) {
        closeSettings();
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
    (void)settingsTab_;
}

void GameApp::drawMapEditorUi() {
    const float s = gui();
    ImGui::SetNextWindowPos(ImVec2{16.0f * s, 16.0f * s}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2{320.0f * s, 420.0f * s}, ImGuiCond_Always);
    ImGui::Begin("##MapEditor", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(s);
    ImGui::TextUnformatted("MAP EDITOR");
    ImGui::TextDisabled("WASD/QE move · RMB look · LMB drag");
    ImGui::Separator();

    if (ImGui::Button("Add Imp", ImVec2{-1.0f, 32.0f * s})) {
        MapObject obj{};
        obj.kind = MapObjectKind::Imp;
        obj.position = camera().position();
        obj.position.y = 0.0f;
        obj.aggroRadius = 16.0f;
        arenaMap_.objects.push_back(obj);
        editorSelection_ = static_cast<int>(arenaMap_.objects.size()) - 1;
    }
    if (ImGui::Button("Add Ink Pool", ImVec2{-1.0f, 32.0f * s})) {
        MapObject obj{};
        obj.kind = MapObjectKind::InkPool;
        obj.position = camera().position();
        obj.position.y = 0.05f;
        obj.radius = 1.5f;
        arenaMap_.objects.push_back(obj);
        editorSelection_ = static_cast<int>(arenaMap_.objects.size()) - 1;
    }
    if (ImGui::Button("Add Ruin", ImVec2{-1.0f, 32.0f * s})) {
        MapObject obj{};
        obj.kind = MapObjectKind::Ruin;
        obj.position = camera().position();
        obj.position.y = 0.5f;
        arenaMap_.objects.push_back(obj);
        editorSelection_ = static_cast<int>(arenaMap_.objects.size()) - 1;
    }

    ImGui::Dummy(ImVec2{0.0f, 8.0f * s});
    if (editorSelection_ == -2) {
        ImGui::TextUnformatted("Selected: Spawn (locked)");
    } else if (editorSelection_ == -1) {
        ImGui::TextUnformatted("Selected: Next Level (locked)");
    } else if (editorSelection_ >= 0 && editorSelection_ < static_cast<int>(arenaMap_.objects.size())) {
        MapObject& obj = arenaMap_.objects[static_cast<size_t>(editorSelection_)];
        ImGui::Text("Selected: %s", mapObjectKindName(obj.kind));
        if (obj.kind == MapObjectKind::Imp) {
            ImGui::SliderFloat("Aggro radius", &obj.aggroRadius, 4.0f, 40.0f, "%.1f");
        }
        if (obj.kind == MapObjectKind::InkPool) {
            ImGui::SliderFloat("Pool radius", &obj.radius, 0.8f, 4.0f, "%.1f");
        }
        if (ImGui::Button("Delete selected", ImVec2{-1.0f, 30.0f * s})) {
            arenaMap_.objects.erase(arenaMap_.objects.begin() + editorSelection_);
            editorSelection_ = -3;
        }
    } else {
        ImGui::TextDisabled("Nothing selected");
    }

    ImGui::Dummy(ImVec2{0.0f, 12.0f * s});
    if (ImGui::Button("Save", ImVec2{-1.0f, 34.0f * s})) {
        saveArenaMap(arenaMapPath(), arenaMap_);
    }
    if (ImGui::Button("Back to Menu", ImVec2{-1.0f, 34.0f * s})) {
        leaveMapEditor(true);
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
}

void GameApp::drawFpsCounter() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float s = gui();
    char label[32];
    std::snprintf(label, sizeof(label), "%.0f FPS", fps());
    const ImVec2 textSize = scaledTextSize(label, s);
    const float pad = 16.0f * s;
    const ImVec2 pos{viewport->WorkPos.x + viewport->WorkSize.x - pad - textSize.x, viewport->WorkPos.y + pad};
    drawScaledText(ImGui::GetBackgroundDrawList(), pos, IM_COL32(220, 220, 210, 210), label, s);
}

void GameApp::drawCurrencyPanel() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float s = gui();
    const float pad = 16.0f * s;
    const float boxW = 150.0f * s;
    const float boxH = 36.0f * s;
    const float gap = 8.0f * s;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();

    const ImVec2 noxMin{viewport->WorkPos.x + pad, viewport->WorkPos.y + pad};
    const ImVec2 noxMax{noxMin.x + boxW, noxMin.y + boxH};
    dl->AddRectFilled(noxMin, noxMax, IM_COL32(28, 24, 36, 220), 4.0f * s);
    dl->AddRect(noxMin, noxMax, IM_COL32(150, 120, 200, 220), 4.0f * s);
    char noxLabel[48];
    std::snprintf(noxLabel, sizeof(noxLabel), "Nox  %d", nox_);
    const ImVec2 noxText = scaledTextSize(noxLabel, s);
    drawScaledText(dl, ImVec2{noxMin.x + (boxW - noxText.x) * 0.5f, noxMin.y + (boxH - noxText.y) * 0.5f},
                   IM_COL32(220, 200, 255, 255), noxLabel, s);

    const ImVec2 pureMin{noxMin.x, noxMax.y + gap};
    const ImVec2 pureMax{pureMin.x + boxW, pureMin.y + boxH};
    dl->AddRectFilled(pureMin, pureMax, IM_COL32(36, 30, 22, 220), 4.0f * s);
    dl->AddRect(pureMin, pureMax, IM_COL32(220, 180, 90, 220), 4.0f * s);
    char pureLabel[48];
    std::snprintf(pureLabel, sizeof(pureLabel), "Pure Nox  %d", pureNox_);
    const ImVec2 pureText = scaledTextSize(pureLabel, s);
    drawScaledText(dl, ImVec2{pureMin.x + (boxW - pureText.x) * 0.5f, pureMin.y + (boxH - pureText.y) * 0.5f},
                   IM_COL32(255, 220, 140, 255), pureLabel, s);
}

void GameApp::drawSkillTreeButton() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float s = gui();
    const float pad = 16.0f * s;
    const float boxW = 150.0f * s;
    const float boxH = 36.0f * s;
    const float gap = 8.0f * s;
    const float icon = 52.0f * s;
    const float y = viewport->WorkPos.y + pad + boxH * 2.0f + gap * 2.0f + 6.0f * s;
    const float x = viewport->WorkPos.x + pad + (boxW - icon) * 0.5f;
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImVec2 min{x, y};
    const ImVec2 max{x + icon, y + icon};
    dl->AddRectFilled(min, max, IM_COL32(24, 28, 34, 230), 6.0f * s);
    dl->AddRect(min, max, skillTreeOpen_ ? IM_COL32(255, 200, 90, 255) : IM_COL32(180, 190, 200, 220), 6.0f * s, 0,
                2.0f * s);
    const char* label = "ST";
    const ImVec2 text = scaledTextSize(label, s * 1.15f);
    drawScaledText(dl, ImVec2{min.x + (icon - text.x) * 0.5f, min.y + (icon - text.y) * 0.5f - 2.0f * s},
                   IM_COL32(235, 230, 220, 255), label, s * 1.15f);
    const char* bind = "TAB";
    const ImVec2 bindSize = scaledTextSize(bind, s * 0.85f);
    drawScaledText(dl, ImVec2{min.x + (icon - bindSize.x) * 0.5f, max.y + 4.0f * s}, IM_COL32(200, 200, 190, 230),
                   bind, s * 0.85f);
}

void GameApp::drawSkillTreePanel() {
    if (!skillTreeOpen_) {
        return;
    }
    const float s = gui();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 winSize{560.0f * s, 480.0f * s};
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(winSize, ImGuiCond_Always);
    ImGui::Begin("##SkillTree", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    ImGui::SetWindowFontScale(s);
    ImGui::TextUnformatted("SKILL TREE");
    ImGui::Text("Nox %d · Pure Nox %d", nox_, pureNox_);
    ImGui::Separator();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = ImGui::GetContentRegionAvail().y - 48.0f * s;

    const ImVec2 start{origin.x + w * 0.5f, origin.y + h * 0.88f};
    const ImVec2 minor1{origin.x + w * 0.28f, origin.y + h * 0.55f};
    const ImVec2 minor2{origin.x + w * 0.72f, origin.y + h * 0.55f};
    const ImVec2 major1{origin.x + w * 0.28f, origin.y + h * 0.22f};
    const ImVec2 major2{origin.x + w * 0.72f, origin.y + h * 0.22f};

    auto line = [&](ImVec2 a, ImVec2 b, bool unlocked) {
        dl->AddLine(a, b, unlocked ? IM_COL32(220, 190, 120, 255) : IM_COL32(90, 90, 100, 200), 3.0f * s);
    };
    line(start, minor1, skills_.minorDashSteer);
    line(start, minor2, skills_.minorBarrierReload);
    line(minor1, major1, skills_.majorDashSpeed);
    line(minor2, major2, skills_.majorBarrierNova);

    struct Node {
        ImVec2 pos;
        const char* title;
        const char* cost;
        bool owned;
        bool available;
        int id;
    };
    const Node nodes[] = {
        {start, "Start", "", true, false, 0},
        {minor1, "Directional\nDash", "2 Nox", skills_.minorDashSteer, !skills_.minorDashSteer && nox_ >= kMinorSkillCost,
         1},
        {minor2, "Barrier\nReload", "2 Nox", skills_.minorBarrierReload,
         !skills_.minorBarrierReload && nox_ >= kMinorSkillCost, 2},
        {major1, "Dash\nMomentum", "5 Pure", skills_.majorDashSpeed,
         skills_.minorDashSteer && !skills_.majorDashSpeed && pureNox_ >= kMajorSkillCost, 3},
        {major2, "Barrier\nNova", "5 Pure", skills_.majorBarrierNova,
         skills_.minorBarrierReload && !skills_.majorBarrierNova && pureNox_ >= kMajorSkillCost, 4},
    };

    const float nodeR = 46.0f * s;
    for (const Node& n : nodes) {
        ImU32 fill = IM_COL32(40, 42, 50, 240);
        ImU32 border = IM_COL32(140, 140, 150, 220);
        if (n.owned) {
            fill = IM_COL32(70, 90, 50, 245);
            border = IM_COL32(180, 220, 100, 255);
        } else if (n.available) {
            fill = IM_COL32(60, 50, 30, 245);
            border = IM_COL32(240, 190, 80, 255);
        }
        dl->AddCircleFilled(n.pos, nodeR, fill, 24);
        dl->AddCircle(n.pos, nodeR, border, 24, 2.0f * s);

        // Multi-line label
        char lineBuf[64];
        const char* p = n.title;
        float lineY = n.pos.y - 18.0f * s;
        while (*p) {
            int li = 0;
            while (*p && *p != '\n' && li < 63) {
                lineBuf[li++] = *p++;
            }
            lineBuf[li] = '\0';
            if (*p == '\n') {
                ++p;
            }
            const ImVec2 ts = scaledTextSize(lineBuf, s * 0.85f);
            drawScaledText(dl, ImVec2{n.pos.x - ts.x * 0.5f, lineY}, IM_COL32(235, 230, 220, 255), lineBuf, s * 0.85f);
            lineY += ts.y + 1.0f * s;
        }
        if (n.cost[0] != '\0') {
            const ImVec2 cs = scaledTextSize(n.cost, s * 0.75f);
            drawScaledText(dl, ImVec2{n.pos.x - cs.x * 0.5f, n.pos.y + 20.0f * s}, IM_COL32(200, 190, 160, 230), n.cost,
                           s * 0.75f);
        }

        if (n.id > 0 && n.available) {
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            const float dx = mouse.x - n.pos.x;
            const float dy = mouse.y - n.pos.y;
            if (dx * dx + dy * dy <= nodeR * nodeR && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                tryUnlockSkill(n.id);
            }
        }
    }

    ImGui::Dummy(ImVec2{0.0f, h});
    if (ImGui::Button("Close (Tab / Esc)", ImVec2{-1.0f, 34.0f * s})) {
        closeSkillTree();
    }
    ImGui::SetWindowFontScale(1.0f);
    ImGui::End();
}

void GameApp::drawPlayerHpBar() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float s = gui();
    const float barW = 220.0f * s;
    const float barH = 18.0f * s;
    const float pad = 24.0f * s;
    const ImVec2 min{viewport->WorkPos.x + pad, viewport->WorkPos.y + viewport->WorkSize.y - pad - barH};
    const ImVec2 max{min.x + barW, min.y + barH};

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(min, max, IM_COL32(20, 20, 24, 210));
    const float pct = std::clamp(player_.hp / player_.maxHp, 0.0f, 1.0f);
    dl->AddRectFilled(min, ImVec2{min.x + barW * pct, max.y}, IM_COL32(200, 55, 55, 235));
    dl->AddRect(min, max, IM_COL32(230, 220, 210, 220));

    char label[48];
    std::snprintf(label, sizeof(label), "HP  %.0f / %.0f", player_.hp, player_.maxHp);
    const ImVec2 textSize = scaledTextSize(label, s);
    drawScaledText(dl, ImVec2{min.x + (barW - textSize.x) * 0.5f, min.y + (barH - textSize.y) * 0.5f},
                   IM_COL32(245, 235, 225, 255), label, s);
}

void GameApp::drawAmmoPips() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float s = gui();
    const float barW = 220.0f * s;
    const float barH = 18.0f * s;
    const float pad = 24.0f * s;
    const float pipH = 10.0f * s;
    const float gap = 4.0f * s;
    const float pipW = (barW - gap * static_cast<float>(player_.clipSize - 1)) / static_cast<float>(player_.clipSize);
    const float baseY = viewport->WorkPos.y + viewport->WorkSize.y - pad - barH - 8.0f * s - pipH;
    const float baseX = viewport->WorkPos.x + pad;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    for (int i = 0; i < player_.clipSize; ++i) {
        const float x = baseX + static_cast<float>(i) * (pipW + gap);
        const ImVec2 min{x, baseY};
        const ImVec2 max{x + pipW, baseY + pipH};
        dl->AddRectFilled(min, max, IM_COL32(28, 26, 22, 210));

        float fill = 0.0f;
        if (player_.reloading) {
            const float progress = 1.0f - std::clamp(player_.reloadTimer / currentReloadDuration(), 0.0f, 1.0f);
            const float slotStart = static_cast<float>(i) / static_cast<float>(player_.clipSize);
            const float slotEnd = static_cast<float>(i + 1) / static_cast<float>(player_.clipSize);
            fill = std::clamp((progress - slotStart) / (slotEnd - slotStart), 0.0f, 1.0f);
        } else {
            const int remaining = player_.clipSize - player_.shotsInClip;
            fill = (i < remaining) ? 1.0f : 0.0f;
        }

        if (fill > 0.0f) {
            dl->AddRectFilled(min, ImVec2{min.x + pipW * fill, max.y}, IM_COL32(235, 190, 55, 235));
        }
        dl->AddRect(min, max, IM_COL32(210, 190, 140, 200));
    }
}

void GameApp::drawAbilitySlots() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float s = gui();
    const float pad = 24.0f * s;
    const float small = 48.0f * s;
    const float big = 68.0f * s;
    const float gap = 8.0f * s;
    const float bindH = 16.0f * s;
    const float totalW = small * 3.0f + gap * 3.0f + big;
    const float baseX = viewport->WorkPos.x + viewport->WorkSize.x - pad - totalW;
    const float baseY = viewport->WorkPos.y + viewport->WorkSize.y - pad - big - bindH;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const float cds[3] = {player_.meleeCooldown, player_.dashCooldown,
                          barrierActive() ? player_.barrierTimer : player_.barrierCooldown};
    const float cdMax[3] = {player_.meleeCooldownDuration, player_.dashCooldownDuration,
                            barrierActive() ? kBarrierDuration : player_.barrierCooldownDuration};
    const char* binds[3] = {"Q", "SHIFT", "E"};

    for (int i = 0; i < 3; ++i) {
        const float x = baseX + static_cast<float>(i) * (small + gap);
        const float y = baseY + (big - small);
        const ImVec2 min{x, y};
        const ImVec2 max{x + small, y + small};
        dl->AddRectFilled(min, max, IM_COL32(22, 22, 28, 220));
        if (abilityIcons_[static_cast<size_t>(i)].descriptor) {
            dl->AddImage(texRef(abilityIcons_[static_cast<size_t>(i)].descriptor), min, max);
        }
        if (cds[i] > 0.0f && cdMax[i] > 0.0f) {
            const float pct = std::clamp(cds[i] / cdMax[i], 0.0f, 1.0f);
            dl->AddRectFilled(ImVec2{min.x, max.y - small * pct}, max, IM_COL32(8, 8, 12, 180));
        }
        dl->AddRect(min, max, IM_COL32(200, 190, 170, 210));
        const ImVec2 bindSize = scaledTextSize(binds[i], s * 0.8f);
        drawScaledText(dl, ImVec2{min.x + (small - bindSize.x) * 0.5f, max.y + 2.0f * s},
                       IM_COL32(210, 205, 195, 230), binds[i], s * 0.8f);
    }

    const float ultX = baseX + 3.0f * (small + gap);
    const ImVec2 umin{ultX, baseY};
    const ImVec2 umax{ultX + big, baseY + big};
    dl->AddRectFilled(umin, umax, IM_COL32(28, 22, 34, 230));
    if (abilityIcons_[3].descriptor) {
        dl->AddImage(texRef(abilityIcons_[3].descriptor), umin, umax);
    }
    if (ultActive()) {
        const float pct = std::clamp(player_.ultTimer / kUltDuration, 0.0f, 1.0f);
        dl->AddRect(umin, umax, IM_COL32(255, 180, 60, 255), 0.0f, 0, 2.5f);
        dl->AddRectFilled(ImVec2{umin.x, umax.y - 4.0f * s}, ImVec2{umin.x + big * pct, umax.y},
                          IM_COL32(255, 170, 50, 230));
    } else if (player_.ultCooldown > 0.0f) {
        const float pct = std::clamp(player_.ultCooldown / player_.ultCooldownDuration, 0.0f, 1.0f);
        dl->AddRectFilled(ImVec2{umin.x, umax.y - big * pct}, umax, IM_COL32(8, 8, 12, 180));
    }
    dl->AddRect(umin, umax, IM_COL32(180, 140, 210, 220));
    const char* ultBind = "SPACE";
    const ImVec2 ultBindSize = scaledTextSize(ultBind, s * 0.8f);
    drawScaledText(dl, ImVec2{umin.x + (big - ultBindSize.x) * 0.5f, umax.y + 2.0f * s},
                   IM_COL32(210, 205, 195, 230), ultBind, s * 0.8f);
}

void GameApp::drawCrosshair() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center{viewport->GetCenter()};
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const float s = gui();
    const float arm = 8.0f * s;
    const float gap = 3.0f * s;
    const float thick = 2.0f * s;
    const ImU32 col = IM_COL32(240, 235, 220, 220);
    dl->AddRectFilled(ImVec2{center.x - arm, center.y - thick * 0.5f}, ImVec2{center.x - gap, center.y + thick * 0.5f},
                      col);
    dl->AddRectFilled(ImVec2{center.x + gap, center.y - thick * 0.5f}, ImVec2{center.x + arm, center.y + thick * 0.5f},
                      col);
    dl->AddRectFilled(ImVec2{center.x - thick * 0.5f, center.y - arm}, ImVec2{center.x + thick * 0.5f, center.y - gap},
                      col);
    dl->AddRectFilled(ImVec2{center.x - thick * 0.5f, center.y + gap}, ImVec2{center.x + thick * 0.5f, center.y + arm},
                      col);
}

void GameApp::drawImpHpBars() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float s = gui();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    for (const ImpEnemy& imp : imps_) {
        if (!imp.alive) {
            continue;
        }
        nu::Vec2 screen{};
        const nu::Vec3 head = imp.position + nu::Vec3{0.0f, 1.85f, 0.0f};
        if (!projectWorldToScreen(camera(), head, viewport->Size.x, viewport->Size.y, screen)) {
            continue;
        }
        screen.x += viewport->Pos.x;
        screen.y += viewport->Pos.y;

        const float barW = 72.0f * s;
        const float barH = 8.0f * s;
        const ImVec2 min{screen.x - barW * 0.5f, screen.y - barH * 0.5f};
        const ImVec2 max{min.x + barW, min.y + barH};
        dl->AddRectFilled(min, max, IM_COL32(20, 20, 24, 200));
        const float pct = std::clamp(imp.hp / imp.maxHp, 0.0f, 1.0f);
        dl->AddRectFilled(min, ImVec2{min.x + barW * pct, max.y}, IM_COL32(180, 40, 50, 230));
        dl->AddRect(min, max, IM_COL32(230, 220, 210, 220));

        char label[32];
        std::snprintf(label, sizeof(label), "Imp %.0f/%.0f", imp.hp, imp.maxHp);
        const ImVec2 textSize = scaledTextSize(label, s);
        drawScaledText(dl, ImVec2{screen.x - textSize.x * 0.5f, min.y - textSize.y - 2.0f * s},
                       IM_COL32(240, 230, 220, 255), label, s);

        if (imp.aggroed) {
            const ImVec2 bangSize = scaledTextSize("!", s * 1.2f);
            drawScaledText(dl, ImVec2{screen.x - bangSize.x * 0.5f, min.y - textSize.y - bangSize.y - 6.0f * s},
                           IM_COL32(255, 220, 80, 255), "!", s * 1.2f);
        }
    }
}

void GameApp::drawDamageNumbers() {
    if (!settings_.showDamageNumbers) {
        return;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float s = gui();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    for (const DamageNumber& dn : damageNumbers_) {
        nu::Vec2 screen{};
        if (!projectWorldToScreen(camera(), dn.worldPos, viewport->Size.x, viewport->Size.y, screen)) {
            continue;
        }
        screen.x += viewport->Pos.x;
        screen.y += viewport->Pos.y;
        const float t = dn.age / dn.lifetime;
        const int alpha = static_cast<int>((1.0f - t) * 255.0f);
        char label[16];
        std::snprintf(label, sizeof(label), "%.0f", dn.amount);
        const ImVec2 textSize = scaledTextSize(label, s);
        drawScaledText(dl, ImVec2{screen.x - textSize.x * 0.5f, screen.y - textSize.y * 0.5f},
                       IM_COL32(255, 210, 120, alpha), label, s);
    }
}

void GameApp::drawDamageVignette() {
    const float hpRatio = std::clamp(player_.hp / player_.maxHp, 0.0f, 1.0f);
    float intensity = 0.0f;
    if (player_.damageFlash > 0.0f) {
        intensity = std::max(intensity, (player_.damageFlash / 0.45f) * (0.25f + (1.0f - hpRatio) * 0.55f));
    }
    if (hpRatio <= kLowHpThreshold && player_.hp > 0.0f) {
        const float low = 1.0f - (hpRatio / kLowHpThreshold);
        intensity = std::max(intensity, 0.18f + low * 0.42f);
    }
    if (intensity <= 0.001f) {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImVec2 min = viewport->WorkPos;
    const ImVec2 max{viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y};
    const int alpha = static_cast<int>(std::clamp(intensity, 0.0f, 0.85f) * 180.0f);
    const ImU32 edge = IM_COL32(160, 10, 10, alpha);
    const ImU32 clear = IM_COL32(160, 10, 10, 0);
    const float band = std::min(viewport->WorkSize.x, viewport->WorkSize.y) * 0.22f;
    dl->AddRectFilledMultiColor(min, ImVec2{max.x, min.y + band}, edge, edge, clear, clear);
    dl->AddRectFilledMultiColor(ImVec2{min.x, max.y - band}, max, clear, clear, edge, edge);
    dl->AddRectFilledMultiColor(min, ImVec2{min.x + band, max.y}, edge, clear, clear, edge);
    dl->AddRectFilledMultiColor(ImVec2{max.x - band, min.y}, max, clear, edge, edge, clear);
}

void GameApp::drawGameplayHud() {
    drawCurrencyPanel();
    drawSkillTreeButton();
    drawFpsCounter();
    drawDamageVignette();
    drawPlayerHpBar();
    drawAmmoPips();
    drawAbilitySlots();
    drawCrosshair();
    drawImpHpBars();
    drawDamageNumbers();
    drawSkillTreePanel();
}

void GameApp::drawBarrierAura(nu::RenderScene& scene) {
    if (!barrierActive()) {
        return;
    }
    nu::DrawItem shell;
    shell.mesh = &playerMesh_;
    shell.transform = glm::translate(nu::Mat4{1.0f}, character().position()) *
                      glm::scale(nu::Mat4{1.0f}, nu::Vec3{1.35f, 1.35f, 1.35f});
    const float pulse = 0.55f + 0.25f * std::sin(static_cast<float>(ImGui::GetTime()) * 8.0f);
    shell.material.baseColor = {0.95f, 0.45f, 0.12f, 0.35f};
    shell.material.emissive = {pulse * 1.4f, pulse * 0.45f, 0.08f, 1.0f};
    shell.material.roughness = 0.2f;
    scene.draws.push_back(shell);
}

void GameApp::drawScorchZones(nu::RenderScene& scene) {
    for (const ScorchZone& z : scorchZones_) {
        const float t = std::clamp(z.remaining / kScorchDuration, 0.0f, 1.0f);
        nu::DrawItem patch;
        patch.mesh = &projectileMesh_;
        patch.transform = glm::translate(nu::Mat4{1.0f}, z.position) *
                          glm::scale(nu::Mat4{1.0f}, nu::Vec3{z.radius * 1.6f, 0.05f, z.radius * 1.6f});
        patch.material.baseColor = {0.35f, 0.08f, 0.04f, 1.0f};
        patch.material.emissive = {0.9f * t, 0.25f * t, 0.05f * t, 1.0f};
        scene.draws.push_back(patch);
    }
}

void GameApp::drawMapObjects(nu::RenderScene& scene, bool editorView) {
    for (size_t i = 0; i < arenaMap_.objects.size(); ++i) {
        const MapObject& obj = arenaMap_.objects[i];
        const bool selected = editorView && editorSelection_ == static_cast<int>(i);
        if (obj.kind == MapObjectKind::InkPool) {
            nu::DrawItem pool;
            pool.mesh = &cubeMesh_;
            pool.transform = glm::translate(nu::Mat4{1.0f}, obj.position) *
                             glm::scale(nu::Mat4{1.0f}, nu::Vec3{obj.radius * 2.0f, 0.05f, obj.radius * 2.0f});
            pool.material.baseColor = {0.08f, 0.06f, 0.12f, 1.0f};
            pool.material.roughness = 0.25f;
            if (selected) {
                pool.material.emissive = {0.35f, 0.25f, 0.55f, 1.0f};
            }
            scene.draws.push_back(pool);
        } else if (obj.kind == MapObjectKind::Ruin) {
            nu::DrawItem block;
            block.mesh = &cubeMesh_;
            block.transform =
                glm::translate(nu::Mat4{1.0f}, obj.position) * glm::scale(nu::Mat4{1.0f}, nu::Vec3{2.0f, 1.0f, 2.0f});
            block.material.baseColor = selected ? nu::Vec4{0.7f, 0.65f, 0.4f, 1.0f} : nu::Vec4{0.5f, 0.48f, 0.46f, 1.0f};
            scene.draws.push_back(block);
        } else if (obj.kind == MapObjectKind::Imp && editorView) {
            nu::DrawItem enemy;
            enemy.mesh = &impMesh_;
            enemy.transform = glm::translate(nu::Mat4{1.0f}, obj.position + nu::Vec3{0.0f, 0.9f, 0.0f});
            enemy.material.baseColor = selected ? nu::Vec4{0.9f, 0.35f, 0.2f, 1.0f} : nu::Vec4{0.55f, 0.18f, 0.22f, 1.0f};
            enemy.material.emissive = selected ? nu::Vec4{0.5f, 0.15f, 0.05f, 1.0f} : nu::Vec4{0.0f};
            scene.draws.push_back(enemy);
        }
    }

    // Compulsory markers
    {
        nu::DrawItem spawn;
        spawn.mesh = &playerMesh_;
        spawn.transform = glm::translate(nu::Mat4{1.0f}, arenaMap_.spawnPoint) *
                          glm::scale(nu::Mat4{1.0f}, nu::Vec3{0.85f, 0.85f, 0.85f});
        const bool sel = editorView && editorSelection_ == -2;
        spawn.material.baseColor = {0.25f, 0.75f, 0.35f, 1.0f};
        spawn.material.emissive = sel ? nu::Vec4{0.2f, 0.8f, 0.3f, 1.0f} : nu::Vec4{0.05f, 0.25f, 0.08f, 1.0f};
        scene.draws.push_back(spawn);
    }
    {
        nu::DrawItem next;
        next.mesh = &cubeMesh_;
        next.transform = glm::translate(nu::Mat4{1.0f}, arenaMap_.nextLevelPoint) *
                         glm::scale(nu::Mat4{1.0f}, nu::Vec3{2.2f, 0.15f, 2.2f});
        const bool sel = editorView && editorSelection_ == -1;
        next.material.baseColor = {0.2f, 0.45f, 0.9f, 1.0f};
        next.material.emissive = sel ? nu::Vec4{0.3f, 0.55f, 1.2f, 1.0f} : nu::Vec4{0.1f, 0.2f, 0.5f, 1.0f};
        scene.draws.push_back(next);
    }
}

void GameApp::drawAggroRadii(nu::RenderScene& scene) {
    for (size_t i = 0; i < arenaMap_.objects.size(); ++i) {
        const MapObject& obj = arenaMap_.objects[i];
        if (obj.kind != MapObjectKind::Imp) {
            continue;
        }
        const bool selected = editorSelection_ == static_cast<int>(i);
        constexpr int kSeg = 24;
        for (int s = 0; s < kSeg; ++s) {
            const float a0 = (static_cast<float>(s) / static_cast<float>(kSeg)) * 6.2831853f;
            const float a1 = (static_cast<float>(s + 1) / static_cast<float>(kSeg)) * 6.2831853f;
            const nu::Vec3 p0{obj.position.x + std::cos(a0) * obj.aggroRadius, 0.08f,
                              obj.position.z + std::sin(a0) * obj.aggroRadius};
            const nu::Vec3 mid = (p0 + nu::Vec3{obj.position.x + std::cos(a1) * obj.aggroRadius, 0.08f,
                                                obj.position.z + std::sin(a1) * obj.aggroRadius}) *
                                 0.5f;
            nu::DrawItem seg;
            seg.mesh = &projectileMesh_;
            seg.transform = glm::translate(nu::Mat4{1.0f}, mid) *
                            glm::scale(nu::Mat4{1.0f}, nu::Vec3{0.35f, 0.05f, 0.35f});
            seg.material.baseColor = {0.9f, 0.25f, 0.2f, 1.0f};
            seg.material.emissive = selected ? nu::Vec4{1.0f, 0.3f, 0.15f, 1.0f} : nu::Vec4{0.4f, 0.08f, 0.05f, 1.0f};
            scene.draws.push_back(seg);
        }
    }
}

void GameApp::onRender(float) {
    nu::RenderScene scene;
    scene.clearColor = {0.18f, 0.19f, 0.22f, 1.0f};
    scene.fogColor = {0.32f, 0.34f, 0.38f};
    scene.fogDensity = 0.022f;
    scene.lightDir = {-0.25f, -0.9f, -0.2f};
    scene.lightColor = {1.35f, 1.05f, 0.75f};
    scene.ambientColor = {0.10f, 0.11f, 0.14f};
    scene.draws = staticDraws_;

    if (urnProp_) {
        for (const nu::Mat4& xf : urnTransforms_) {
            urnProp_->appendDraws(scene.draws, xf, 0);
        }
    }

    const bool editorView = screen_ == AppScreen::MapEditor;
    drawMapObjects(scene, editorView || screen_ == AppScreen::MainMenu);
    if (editorView) {
        drawAggroRadii(scene);
    }

    if (!editorView) {
        drawScorchZones(scene);
        for (const ImpEnemy& imp : imps_) {
            if (!imp.alive) {
                continue;
            }
            nu::DrawItem impDraw;
            impDraw.mesh = &impMesh_;
            impDraw.transform = glm::translate(nu::Mat4{1.0f}, imp.position + nu::Vec3{0.0f, 0.9f, 0.0f});
            if (imp.windupFlashTimer > 0.0f) {
                const float flash = imp.windupFlashTimer / kImpWindupFlashDuration;
                impDraw.material.baseColor = {0.95f, 0.88f, 0.20f + 0.15f * flash, 1.0f};
                impDraw.material.emissive = {1.6f * flash, 1.35f * flash, 0.15f * flash, 1.0f};
            } else if (imp.hitFlashTimer > 0.0f) {
                const float flash = imp.hitFlashTimer / kHitFlashDuration;
                impDraw.material.baseColor = {0.95f, 0.55f + 0.35f * flash, 0.45f, 1.0f};
                impDraw.material.emissive = {1.4f * flash, 0.45f * flash, 0.2f * flash, 1.0f};
            } else if (!imp.burnStacks.empty()) {
                impDraw.material.baseColor = {0.7f, 0.22f, 0.12f, 1.0f};
                impDraw.material.emissive = {0.55f, 0.18f, 0.04f, 1.0f};
            } else if (imp.aggroed) {
                impDraw.material.baseColor = {0.62f, 0.16f, 0.18f, 1.0f};
                impDraw.material.emissive = {0.15f, 0.02f, 0.02f, 1.0f};
            } else {
                impDraw.material.baseColor = {0.55f, 0.18f, 0.22f, 1.0f};
            }
            impDraw.material.roughness = 0.65f;
            scene.draws.push_back(impDraw);
        }

        for (const Projectile& proj : projectiles_) {
            nu::DrawItem shot;
            shot.mesh = &projectileMesh_;
            const float scale = proj.owner == ProjectileOwner::Player ? 0.22f : 0.26f;
            shot.transform =
                glm::translate(nu::Mat4{1.0f}, proj.position) * glm::scale(nu::Mat4{1.0f}, nu::Vec3{scale});
            if (proj.owner == ProjectileOwner::Player) {
                shot.material.baseColor = {1.0f, 0.55f, 0.15f, 1.0f};
                shot.material.emissive = {1.8f, 0.65f, 0.12f, 1.0f};
            } else {
                shot.material.baseColor = {0.35f, 0.85f, 0.28f, 1.0f};
                shot.material.emissive = {0.25f, 1.1f, 0.2f, 1.0f};
            }
            scene.draws.push_back(shot);
        }

        for (const TrailParticle& p : trailParticles_) {
            const float t = std::clamp(p.age / p.lifetime, 0.0f, 1.0f);
            const float sc = p.scale * (1.0f - t * 0.7f);
            nu::DrawItem spark;
            spark.mesh = &projectileMesh_;
            spark.transform = glm::translate(nu::Mat4{1.0f}, p.position) * glm::scale(nu::Mat4{1.0f}, nu::Vec3{sc});
            spark.material.baseColor = {1.0f, 0.45f + 0.3f * (1.0f - t), 0.08f, 1.0f};
            spark.material.emissive = {(1.0f - t) * 1.4f, (1.0f - t) * 0.4f, 0.05f, 1.0f};
            scene.draws.push_back(spark);
        }

        for (const MeleeSlashVfx& slash : meleeVfx_) {
            const float t = slash.age / slash.lifetime;
            const nu::Vec3 right = glm::normalize(glm::cross(slash.forward, nu::Vec3{0.0f, 1.0f, 0.0f}));
            nu::Mat4 basis{1.0f};
            basis[0] = nu::Vec4{right * (1.9f * (1.0f - t * 0.35f)), 0.0f};
            basis[1] = nu::Vec4{0.0f, 0.28f, 0.0f, 0.0f};
            basis[2] = nu::Vec4{slash.forward * 0.22f, 0.0f};
            nu::DrawItem arc;
            arc.mesh = &projectileMesh_;
            arc.transform = glm::translate(nu::Mat4{1.0f}, slash.position) * basis;
            arc.material.baseColor = {0.95f, 0.85f, 0.55f, 1.0f};
            arc.material.emissive = {(1.0f - t) * 1.2f, (1.0f - t) * 0.9f, (1.0f - t) * 0.3f, 1.0f};
            scene.draws.push_back(arc);
        }
    }

    if (screen_ == AppScreen::Playing || screen_ == AppScreen::Paused || screen_ == AppScreen::Death ||
        (screen_ == AppScreen::Settings && settingsReturn_ == AppScreen::Paused)) {
        nu::DrawItem player;
        player.mesh = &playerMesh_;
        player.transform = glm::translate(nu::Mat4{1.0f}, character().position());
        player.material.baseColor =
            ultActive() ? nu::Vec4{0.95f, 0.45f, 0.18f, 1.0f} : nu::Vec4{0.72f, 0.28f, 0.22f, 1.0f};
        player.material.emissive = ultActive() ? nu::Vec4{0.7f, 0.25f, 0.05f, 1.0f} : nu::Vec4{0.0f};
        player.material.roughness = 0.7f;
        scene.draws.push_back(player);
        drawBarrierAura(scene);
    }

    renderer().beginFrame();
    renderer().drawScene(camera(), scene);

    imgui().beginFrame();
    switch (screen_) {
    case AppScreen::MainMenu:
        drawMainMenu();
        drawFpsCounter();
        break;
    case AppScreen::Playing:
        drawGameplayHud();
        break;
    case AppScreen::Paused:
        drawGameplayHud();
        drawPauseMenu();
        break;
    case AppScreen::Death:
        drawGameplayHud();
        drawDeathMenu();
        break;
    case AppScreen::Settings:
        if (settingsReturn_ == AppScreen::Paused) {
            drawGameplayHud();
        }
        drawSettingsMenu();
        drawFpsCounter();
        break;
    case AppScreen::MapEditor:
        drawMapEditorUi();
        drawFpsCounter();
        break;
    }

    renderer().endFrame([this](VkCommandBuffer cmd) { imgui().endFrame(cmd); });
}

} // namespace nine
