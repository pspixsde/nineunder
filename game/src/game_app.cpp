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
constexpr float kImpAggroRadius = 16.0f;
constexpr float kImpDesiredRange = 7.5f;
constexpr float kImpAttackRange = 11.0f;
constexpr float kImpAttackInterval = 1.35f;
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
constexpr float kBarrierDuration = 4.0f;
constexpr float kUltDuration = 10.0f;
constexpr float kUltFireInterval = 0.12f;
constexpr float kUltReloadDuration = 0.65f;
constexpr float kUltFireDamage = 80.0f;
constexpr float kBurnDuration = 2.0f;
constexpr float kBurnDps = 10.0f;
constexpr float kLowHpThreshold = 0.25f;

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

} // namespace

GameApp::GameApp() : nu::Application(nu::ApplicationDesc{"Nine Under", 1600, 900, false}) {}

void GameApp::onInit() {
    NU_GAME_INFO("Game init — Arena I (The Grey Threshold) v0.4");
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

void GameApp::buildArenaOne() {
    auto& factory = renderer().meshes();
    floorMesh_ = factory.createPlane(80.0f, 8);
    cubeMesh_ = factory.createCube(1.0f);
    wallMesh_ = factory.createCube(1.0f);
    playerMesh_ = factory.createCapsule(0.4f, 0.5f, 10);
    impMesh_ = factory.createCapsule(0.35f, 0.45f, 10);
    projectileMesh_ = factory.createCube(1.0f);
    meshesReady_ = true;

    const ArenaGreybox arena =
        buildArenaOneGreybox(factory, physics(), floorMesh_, cubeMesh_, wallMesh_);
    staticDraws_ = arena.draws;
    NU_GAME_INFO("Arena I graybox ready ({} draw items)", staticDraws_.size());
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
}

bool GameApp::isSimulating() const {
    return screen_ == AppScreen::Playing;
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

float GameApp::impMoveMultiplier() const {
    for (const ScorchZone& z : scorchZones_) {
        if (flatDistance(imp_.position, z.position) <= z.radius) {
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

void GameApp::enterMainMenu() {
    screen_ = AppScreen::MainMenu;
    gameplayActive_ = false;
    freeCameraMode_ = false;
    camera().setMode(nu::CameraMode::ThirdPerson);
    input().setRelativeMouse(false);
}

void GameApp::openSettings(AppScreen returnScreen) {
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
    resetEncounter();
    screen_ = AppScreen::Playing;
    gameplayActive_ = true;
    input().setRelativeMouse(true);
}

void GameApp::pauseGame() {
    if (screen_ != AppScreen::Playing) {
        return;
    }
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

void GameApp::resetEncounter() {
    player_ = PlayerCombat{};
    projectiles_.clear();
    scorchZones_.clear();
    trailParticles_.clear();
    meleeVfx_.clear();
    damageNumbers_.clear();
    character().setPosition({0.0f, 2.0f, 0.0f});
    camera().yaw = -40.0f;
    camera().pitch = -20.0f;
    camera().setTarget(character().position());
    camera().update();
    respawnImp();
}

void GameApp::pickImpRoamTarget() {
    std::uniform_real_distribution<float> dist(-6.0f, 6.0f);
    imp_.roamTarget = imp_.spawnPosition + nu::Vec3{dist(rng_), 0.0f, dist(rng_)};
    imp_.roamRetargetTimer = std::uniform_real_distribution<float>(2.5f, 4.5f)(rng_);
}

void GameApp::respawnImp() {
    imp_ = ImpEnemy{};
    imp_.spawnPosition = {18.0f, 0.0f, 14.0f};
    imp_.position = imp_.spawnPosition;
    imp_.hp = imp_.maxHp;
    imp_.attackCooldown = 0.6f;
    pickImpRoamTarget();
}

void GameApp::killImp() {
    if (!imp_.alive) {
        return;
    }
    imp_.hp = 0.0f;
    imp_.alive = false;
    imp_.aggroed = false;
    imp_.burnStacks.clear();

    std::uniform_real_distribution<float> pitchDist(0.88f, 1.12f);
    audio().playSound(nu::Paths::resolveData("audio/sfx/imp_death.wav"), pitchDist(rng_));
    NU_GAME_INFO("Imp died");
}

void GameApp::killPlayer() {
    if (screen_ != AppScreen::Playing && screen_ != AppScreen::Paused) {
        return;
    }
    player_.hp = 0.0f;
    screen_ = AppScreen::Death;
    gameplayActive_ = false;
    input().setRelativeMouse(false);
    projectiles_.clear();
    NU_GAME_INFO("Player died");
}

void GameApp::onEscapePressed() {
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

void GameApp::spawnDamageNumber(const nu::Vec3& worldPos, float amount) {
    if (!settings_.showDamageNumbers) {
        return;
    }
    DamageNumber dn;
    dn.worldPos = worldPos;
    dn.amount = amount;
    damageNumbers_.push_back(dn);
}

void GameApp::damageImp(float amount, const nu::Vec3& hitPos, bool applyBurn) {
    if (!imp_.alive) {
        return;
    }
    imp_.hp = std::max(0.0f, imp_.hp - amount);
    imp_.hitFlashTimer = kHitFlashDuration;
    spawnDamageNumber(hitPos + nu::Vec3{0.0f, 0.35f, 0.0f}, amount);

    if (applyBurn) {
        imp_.burnStacks.push_back(kBurnDuration);
    }

    if (!imp_.aggroed) {
        imp_.aggroed = true;
        imp_.ai = ImpAiState::Aggro;
        imp_.noticeTimer = 1.4f;
    }

    if (imp_.hp <= 0.0f) {
        killImp();
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
    if (player_.fireCooldown > 0.0f) {
        return;
    }
    if (!barrierActive() && player_.reloading) {
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

    if (!barrierActive()) {
        ++player_.shotsInClip;
        if (player_.shotsInClip >= player_.clipSize) {
            player_.reloading = true;
            player_.reloadTimer = currentReloadDuration();
            player_.shotsInClip = 0;
        }
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

    if (!imp_.alive) {
        return;
    }

    const float dist = flatDistance(origin, imp_.position);
    if (dist > kMeleeRange || dist < 1.0e-3f) {
        return;
    }
    const nu::Vec3 toFlat = flatDir(origin, imp_.position);
    if (glm::dot(forward, toFlat) < kMeleeHalfAngleCos) {
        return;
    }

    damageImp(kMeleeDamage, imp_.position + nu::Vec3{0.0f, 1.2f, 0.0f}, ultActive());
    if (imp_.alive) {
        imp_.position += toFlat * kMeleeKnockback;
    }
    audio().playSound(nu::Paths::resolveData("audio/sfx/fireball_hit.wav"),
                      std::uniform_real_distribution<float>(0.75f, 0.9f)(rng_));
}

void GameApp::tryDash() {
    if (player_.dashCooldown > 0.0f) {
        return;
    }
    player_.dashCooldown = player_.dashCooldownDuration;

    const nu::Vec3 forward = playerFlatForward();
    const nu::Vec3 start = character().position();
    const nu::Vec3 end = start + forward * kDashDistance;
    character().setPosition(end);

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
    player_.reloading = false;
    player_.reloadTimer = 0.0f;
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

void GameApp::spawnImpSpit() {
    const nu::Vec3 start = imp_.position + nu::Vec3{0.0f, 1.25f, 0.0f};
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

void GameApp::updateImpAi(float dt) {
    if (!imp_.alive) {
        return;
    }

    if (imp_.hitFlashTimer > 0.0f) {
        imp_.hitFlashTimer = std::max(0.0f, imp_.hitFlashTimer - dt);
    }
    if (imp_.attackCooldown > 0.0f) {
        imp_.attackCooldown = std::max(0.0f, imp_.attackCooldown - dt);
    }
    if (imp_.noticeTimer > 0.0f) {
        imp_.noticeTimer = std::max(0.0f, imp_.noticeTimer - dt);
    }

    const nu::Vec3 playerPos = character().position();
    const float dist = flatDistance(imp_.position, playerPos);
    const float moveMul = impMoveMultiplier();

    if (!imp_.aggroed && dist <= kImpAggroRadius) {
        imp_.aggroed = true;
        imp_.ai = ImpAiState::Aggro;
        imp_.noticeTimer = 1.6f;
    }

    if (imp_.ai == ImpAiState::Roam) {
        imp_.roamRetargetTimer -= dt;
        if (imp_.roamRetargetTimer <= 0.0f || flatDistance(imp_.position, imp_.roamTarget) < 0.4f) {
            pickImpRoamTarget();
        }
        imp_.position += flatDir(imp_.position, imp_.roamTarget) * (kImpRoamSpeed * moveMul * dt);
        return;
    }

    const float rangeError = dist - kImpDesiredRange;
    if (std::abs(rangeError) > 0.35f) {
        const nu::Vec3 toward = flatDir(imp_.position, playerPos);
        const nu::Vec3 moveDir = rangeError > 0.0f ? toward : -toward;
        const float speed = rangeError > 0.0f ? kImpChaseSpeed : kImpRetreatSpeed;
        imp_.position += moveDir * (speed * moveMul * dt);
    }

    if (dist <= kImpAttackRange && imp_.attackCooldown <= 0.0f) {
        spawnImpSpit();
        imp_.attackCooldown = kImpAttackInterval;
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
    if (player_.barrierTimer > 0.0f) {
        player_.barrierTimer = std::max(0.0f, player_.barrierTimer - dt);
        if (player_.barrierTimer <= 0.0f || player_.barrierHp <= 0.0f) {
            player_.barrierTimer = 0.0f;
            player_.barrierHp = 0.0f;
        }
    }
    if (player_.ultTimer > 0.0f) {
        player_.ultTimer = std::max(0.0f, player_.ultTimer - dt);
    }
    if (player_.damageFlash > 0.0f) {
        player_.damageFlash = std::max(0.0f, player_.damageFlash - dt);
    }

    if (!barrierActive() && player_.reloading) {
        player_.reloadTimer -= dt;
        if (player_.reloadTimer <= 0.0f) {
            player_.reloading = false;
            player_.reloadTimer = 0.0f;
        }
    }

    if (input().mouseDown(nu::MouseButton::Left)) {
        tryFireProjectile();
    }
    if (input().keyPressed(nu::Key::Q)) {
        tryMeleeAttack();
    }
    if (input().keyPressed(nu::Key::E)) {
        tryDash();
    }
    if (input().keyPressed(nu::Key::R)) {
        tryBarrier();
    }
    if (input().keyPressed(nu::Key::F)) {
        tryUltimate();
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
            if (imp_.alive && pointHitsCapsule(proj.position, imp_.position, kImpRadius, kImpHalfHeight, proj.radius)) {
                damageImp(proj.damage, proj.position, proj.appliesBurn);
                audio().playSound(nu::Paths::resolveData("audio/sfx/fireball_hit.wav"),
                                  std::uniform_real_distribution<float>(0.92f, 1.08f)(rng_));
                proj.lifetime = 0.0f;
            }
        } else if (pointHitsCapsule(proj.position, character().position(), kPlayerRadius, kPlayerHalfHeight,
                                    proj.radius)) {
            if (barrierActive()) {
                player_.barrierHp -= proj.damage;
                if (player_.barrierHp <= 0.0f) {
                    player_.barrierHp = 0.0f;
                    player_.barrierTimer = 0.0f;
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

    if (imp_.alive && impMoveMultiplier() < 1.0f) {
        imp_.scorchDpsAccum += kScorchDps * dt;
        if (imp_.scorchDpsAccum >= 1.0f) {
            const float chunk = std::floor(imp_.scorchDpsAccum);
            imp_.scorchDpsAccum -= chunk;
            damageImp(chunk, imp_.position + nu::Vec3{0.0f, 1.0f, 0.0f});
        }
    } else {
        imp_.scorchDpsAccum = 0.0f;
    }

    if (imp_.alive && !imp_.burnStacks.empty()) {
        float burnDamage = 0.0f;
        for (float& stack : imp_.burnStacks) {
            burnDamage += kBurnDps * dt;
            stack -= dt;
        }
        imp_.burnStacks.erase(std::remove_if(imp_.burnStacks.begin(), imp_.burnStacks.end(),
                                             [](float t) { return t <= 0.0f; }),
                              imp_.burnStacks.end());
        imp_.burnDpsAccum += burnDamage;
        if (imp_.burnDpsAccum >= 1.0f) {
            const float chunk = std::floor(imp_.burnDpsAccum);
            imp_.burnDpsAccum -= chunk;
            damageImp(chunk, imp_.position + nu::Vec3{0.0f, 1.4f, 0.0f});
        }
    } else {
        imp_.burnDpsAccum = 0.0f;
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

void GameApp::onFixedUpdate(float dt) {
    if (!isSimulating()) {
        gameplayActive_ = false;
        Application::onFixedUpdate(dt);
        return;
    }

    gameplayActive_ = true;
    Application::onFixedUpdate(dt);
    updateCombat(dt);
    updateImpAi(dt);
    updateProjectiles(dt);
    updateZonesAndBurns(dt);
    updateVfx(dt);
    updateDamageNumbers(dt);
}

void GameApp::onVariableUpdate(float dt) {
    Application::onVariableUpdate(dt);
    hotReload_.poll();
    (void)dt;
}

void GameApp::drawMainMenu() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{360.0f, 280.0f}, ImGuiCond_Always);
    ImGui::Begin("##MainMenu", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    ImGui::Dummy(ImVec2{0.0f, 12.0f});
    const float titleWidth = ImGui::CalcTextSize("NINE UNDER").x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleWidth) * 0.5f);
    ImGui::TextUnformatted("NINE UNDER");
    ImGui::Dummy(ImVec2{0.0f, 4.0f});
    const float subWidth = ImGui::CalcTextSize("The Grey Threshold").x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - subWidth) * 0.5f);
    ImGui::TextDisabled("The Grey Threshold");
    ImGui::Dummy(ImVec2{0.0f, 20.0f});

    const float btnW = 220.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Play", ImVec2{btnW, 40.0f})) {
        startPlay();
    }
    ImGui::Dummy(ImVec2{0.0f, 8.0f});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Settings", ImVec2{btnW, 36.0f})) {
        openSettings(AppScreen::MainMenu);
    }
    ImGui::Dummy(ImVec2{0.0f, 8.0f});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Quit", ImVec2{btnW, 36.0f})) {
        requestQuit();
    }

    ImGui::End();
}

void GameApp::drawPauseMenu() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{360.0f, 280.0f}, ImGuiCond_Always);
    ImGui::Begin("##PauseMenu", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    ImGui::Dummy(ImVec2{0.0f, 16.0f});
    const float titleWidth = ImGui::CalcTextSize("PAUSED").x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleWidth) * 0.5f);
    ImGui::TextUnformatted("PAUSED");
    ImGui::Dummy(ImVec2{0.0f, 24.0f});

    const float btnW = 220.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Resume", ImVec2{btnW, 40.0f})) {
        resumeGame();
    }
    ImGui::Dummy(ImVec2{0.0f, 8.0f});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Settings", ImVec2{btnW, 36.0f})) {
        openSettings(AppScreen::Paused);
    }
    ImGui::Dummy(ImVec2{0.0f, 8.0f});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Main Menu", ImVec2{btnW, 36.0f})) {
        enterMainMenu();
    }

    ImGui::End();
}

void GameApp::drawDeathMenu() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{360.0f, 240.0f}, ImGuiCond_Always);
    ImGui::Begin("##DeathMenu", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    ImGui::Dummy(ImVec2{0.0f, 16.0f});
    const float titleWidth = ImGui::CalcTextSize("YOU DIED").x;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - titleWidth) * 0.5f);
    ImGui::TextUnformatted("YOU DIED");
    ImGui::Dummy(ImVec2{0.0f, 28.0f});

    const float btnW = 220.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Play Again", ImVec2{btnW, 40.0f})) {
        startPlay();
    }
    ImGui::Dummy(ImVec2{0.0f, 10.0f});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Main Menu", ImVec2{btnW, 40.0f})) {
        enterMainMenu();
    }

    ImGui::End();
}

void GameApp::drawSettingsMenu() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{480.0f, 360.0f}, ImGuiCond_Always);
    ImGui::Begin("##Settings", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

    ImGui::TextUnformatted("SETTINGS");
    ImGui::Separator();
    if (ImGui::BeginTabBar("##SettingsTabs")) {
        if (ImGui::BeginTabItem("General")) {
            settingsTab_ = SettingsTab::General;
            ImGui::Dummy(ImVec2{0.0f, 8.0f});
            ImGui::SliderFloat("Mouse sensitivity", &settings_.mouseSensitivity, 0.2f, 2.5f, "%.2f");
            ImGui::Checkbox("Show floating damage numbers", &settings_.showDamageNumbers);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Audio")) {
            settingsTab_ = SettingsTab::Audio;
            ImGui::Dummy(ImVec2{0.0f, 8.0f});
            if (ImGui::SliderFloat("Volume", &settings_.masterVolume, 0.0f, 1.0f, "%.2f")) {
                audio().setMasterVolume(settings_.masterVolume);
            }
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Credits")) {
            settingsTab_ = SettingsTab::Credits;
            ImGui::Dummy(ImVec2{0.0f, 8.0f});
            ImGui::TextWrapped(
                "Audio assets courtesy of Zapsplat (www.zapsplat.com). Thank you for the SFX library.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::Dummy(ImVec2{0.0f, 16.0f});
    if (ImGui::Button("Back", ImVec2{140.0f, 34.0f})) {
        closeSettings();
    }

    ImGui::End();
    (void)settingsTab_;
}

void GameApp::drawFpsCounter() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    char label[32];
    std::snprintf(label, sizeof(label), "%.0f FPS", fps());
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const float pad = 16.0f;
    const ImVec2 pos{viewport->WorkPos.x + viewport->WorkSize.x - pad - textSize.x, viewport->WorkPos.y + pad};
    ImGui::GetBackgroundDrawList()->AddText(pos, IM_COL32(220, 220, 210, 210), label);
}

void GameApp::drawPlayerHpBar() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float barW = 220.0f;
    const float barH = 18.0f;
    const float pad = 24.0f;
    const ImVec2 min{viewport->WorkPos.x + pad, viewport->WorkPos.y + viewport->WorkSize.y - pad - barH};
    const ImVec2 max{min.x + barW, min.y + barH};

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(min, max, IM_COL32(20, 20, 24, 210));
    const float pct = std::clamp(player_.hp / player_.maxHp, 0.0f, 1.0f);
    dl->AddRectFilled(min, ImVec2{min.x + barW * pct, max.y}, IM_COL32(200, 55, 55, 235));
    dl->AddRect(min, max, IM_COL32(230, 220, 210, 220));

    char label[48];
    std::snprintf(label, sizeof(label), "HP  %.0f / %.0f", player_.hp, player_.maxHp);
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2{min.x + (barW - textSize.x) * 0.5f, min.y + (barH - textSize.y) * 0.5f},
                IM_COL32(245, 235, 225, 255), label);
}

void GameApp::drawAmmoPips() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float barW = 220.0f;
    const float barH = 18.0f;
    const float pad = 24.0f;
    const float pipH = 10.0f;
    const float gap = 4.0f;
    const float pipW = (barW - gap * static_cast<float>(player_.clipSize - 1)) / static_cast<float>(player_.clipSize);
    const float baseY = viewport->WorkPos.y + viewport->WorkSize.y - pad - barH - 8.0f - pipH;
    const float baseX = viewport->WorkPos.x + pad;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    for (int i = 0; i < player_.clipSize; ++i) {
        const float x = baseX + static_cast<float>(i) * (pipW + gap);
        const ImVec2 min{x, baseY};
        const ImVec2 max{x + pipW, baseY + pipH};
        dl->AddRectFilled(min, max, IM_COL32(28, 26, 22, 210));

        float fill = 0.0f;
        if (barrierActive()) {
            fill = 1.0f;
        } else if (player_.reloading) {
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
    const float pad = 24.0f;
    const float small = 48.0f;
    const float big = 68.0f;
    const float gap = 8.0f;
    const float totalW = small * 3.0f + gap * 3.0f + big;
    const float baseX = viewport->WorkPos.x + viewport->WorkSize.x - pad - totalW;
    const float baseY = viewport->WorkPos.y + viewport->WorkSize.y - pad - big;

    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const float cds[3] = {player_.meleeCooldown, player_.dashCooldown,
                          barrierActive() ? player_.barrierTimer : player_.barrierCooldown};
    const float cdMax[3] = {player_.meleeCooldownDuration, player_.dashCooldownDuration,
                            barrierActive() ? kBarrierDuration : player_.barrierCooldownDuration};
    const char* fallback[3] = {"Q", "E", "R"};

    for (int i = 0; i < 3; ++i) {
        const float x = baseX + static_cast<float>(i) * (small + gap);
        const float y = baseY + (big - small);
        const ImVec2 min{x, y};
        const ImVec2 max{x + small, y + small};
        dl->AddRectFilled(min, max, IM_COL32(22, 22, 28, 220));
        if (abilityIcons_[static_cast<size_t>(i)].descriptor) {
            dl->AddImage(texRef(abilityIcons_[static_cast<size_t>(i)].descriptor), min, max);
        } else {
            const ImVec2 textSize = ImGui::CalcTextSize(fallback[i]);
            dl->AddText(ImVec2{min.x + (small - textSize.x) * 0.5f, min.y + (small - textSize.y) * 0.5f},
                        IM_COL32(235, 225, 210, 230), fallback[i]);
        }
        if (cds[i] > 0.0f && cdMax[i] > 0.0f) {
            const float pct = std::clamp(cds[i] / cdMax[i], 0.0f, 1.0f);
            dl->AddRectFilled(ImVec2{min.x, max.y - small * pct}, max, IM_COL32(8, 8, 12, 180));
        }
        dl->AddRect(min, max, IM_COL32(200, 190, 170, 210));
    }

    const float ultX = baseX + 3.0f * (small + gap);
    const ImVec2 umin{ultX, baseY};
    const ImVec2 umax{ultX + big, baseY + big};
    dl->AddRectFilled(umin, umax, IM_COL32(28, 22, 34, 230));
    if (abilityIcons_[3].descriptor) {
        dl->AddImage(texRef(abilityIcons_[3].descriptor), umin, umax);
    } else {
        const ImVec2 ultText = ImGui::CalcTextSize("F");
        dl->AddText(ImVec2{umin.x + (big - ultText.x) * 0.5f, umin.y + (big - ultText.y) * 0.5f},
                    IM_COL32(210, 180, 230, 200), "F");
    }
    if (ultActive()) {
        const float pct = std::clamp(player_.ultTimer / kUltDuration, 0.0f, 1.0f);
        dl->AddRect(umin, umax, IM_COL32(255, 180, 60, 255), 0.0f, 0, 2.5f);
        dl->AddRectFilled(ImVec2{umin.x, umax.y - 4.0f}, ImVec2{umin.x + big * pct, umax.y},
                          IM_COL32(255, 170, 50, 230));
    } else if (player_.ultCooldown > 0.0f) {
        const float pct = std::clamp(player_.ultCooldown / player_.ultCooldownDuration, 0.0f, 1.0f);
        dl->AddRectFilled(ImVec2{umin.x, umax.y - big * pct}, umax, IM_COL32(8, 8, 12, 180));
    }
    dl->AddRect(umin, umax, IM_COL32(180, 140, 210, 220));
}

void GameApp::drawCrosshair() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 center{viewport->GetCenter()};
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const float arm = 8.0f;
    const float gap = 3.0f;
    const float thick = 2.0f;
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

void GameApp::drawImpHpBar() {
    if (!imp_.alive) {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    nu::Vec2 screen{};
    const nu::Vec3 head = imp_.position + nu::Vec3{0.0f, 1.85f, 0.0f};
    if (!projectWorldToScreen(camera(), head, viewport->Size.x, viewport->Size.y, screen)) {
        return;
    }
    screen.x += viewport->Pos.x;
    screen.y += viewport->Pos.y;

    const float barW = 72.0f;
    const float barH = 8.0f;
    const ImVec2 min{screen.x - barW * 0.5f, screen.y - barH * 0.5f};
    const ImVec2 max{min.x + barW, min.y + barH};
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->AddRectFilled(min, max, IM_COL32(20, 20, 24, 200));
    const float pct = std::clamp(imp_.hp / imp_.maxHp, 0.0f, 1.0f);
    dl->AddRectFilled(min, ImVec2{min.x + barW * pct, max.y}, IM_COL32(180, 40, 50, 230));
    dl->AddRect(min, max, IM_COL32(230, 220, 210, 220));

    char label[32];
    std::snprintf(label, sizeof(label), "Imp %.0f/%.0f", imp_.hp, imp_.maxHp);
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2{screen.x - textSize.x * 0.5f, min.y - textSize.y - 2.0f}, IM_COL32(240, 230, 220, 255),
                label);

    if (imp_.aggroed && imp_.noticeTimer > 0.0f) {
        const ImVec2 bangSize = ImGui::CalcTextSize("!");
        dl->AddText(ImVec2{screen.x - bangSize.x * 0.5f, min.y - textSize.y - bangSize.y - 6.0f},
                    IM_COL32(255, 220, 80, 255), "!");
    }
}

void GameApp::drawDamageNumbers() {
    if (!settings_.showDamageNumbers) {
        return;
    }
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
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
        const ImVec2 textSize = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2{screen.x - textSize.x * 0.5f, screen.y - textSize.y * 0.5f},
                    IM_COL32(255, 210, 120, alpha), label);
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
    ImGui::Begin("Nine Under - Arena I");
    ImGui::TextUnformatted("The Grey Threshold");
    ImGui::Separator();
    ImGui::TextUnformatted("LMB fire | Q melee | E dash | R barrier | F ult");
    ImGui::TextUnformatted("F1 free cam | Tab mouse | Esc pause");
    ImGui::Text("Player HP: %.0f / %.0f | Imp burns: %zu", player_.hp, player_.maxHp, imp_.burnStacks.size());
    ImGui::End();

    drawFpsCounter();
    drawDamageVignette();
    drawPlayerHpBar();
    drawAmmoPips();
    drawAbilitySlots();
    drawCrosshair();
    drawImpHpBar();
    drawDamageNumbers();
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

void GameApp::onRender(float) {
    nu::RenderScene scene;
    scene.clearColor = {0.18f, 0.19f, 0.22f, 1.0f};
    scene.fogColor = {0.32f, 0.34f, 0.38f};
    scene.fogDensity = 0.028f;
    scene.lightDir = {-0.25f, -0.9f, -0.2f};
    scene.lightColor = {1.35f, 1.05f, 0.75f};
    scene.ambientColor = {0.10f, 0.11f, 0.14f};
    scene.draws = staticDraws_;

    if (urnProp_) {
        for (const nu::Mat4& xf : urnTransforms_) {
            urnProp_->appendDraws(scene.draws, xf, 0);
        }
    }

    drawScorchZones(scene);

    if (imp_.alive) {
        nu::DrawItem impDraw;
        impDraw.mesh = &impMesh_;
        impDraw.transform = glm::translate(nu::Mat4{1.0f}, imp_.position + nu::Vec3{0.0f, 0.9f, 0.0f});
        if (imp_.hitFlashTimer > 0.0f) {
            const float flash = imp_.hitFlashTimer / kHitFlashDuration;
            impDraw.material.baseColor = {0.95f, 0.55f + 0.35f * flash, 0.45f, 1.0f};
            impDraw.material.emissive = {1.4f * flash, 0.45f * flash, 0.2f * flash, 1.0f};
        } else if (!imp_.burnStacks.empty()) {
            impDraw.material.baseColor = {0.7f, 0.22f, 0.12f, 1.0f};
            impDraw.material.emissive = {0.55f, 0.18f, 0.04f, 1.0f};
        } else if (imp_.aggroed) {
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
        shot.transform = glm::translate(nu::Mat4{1.0f}, proj.position) * glm::scale(nu::Mat4{1.0f}, nu::Vec3{scale});
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
        const float s = p.scale * (1.0f - t * 0.7f);
        nu::DrawItem spark;
        spark.mesh = &projectileMesh_;
        spark.transform = glm::translate(nu::Mat4{1.0f}, p.position) * glm::scale(nu::Mat4{1.0f}, nu::Vec3{s});
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

    if (screen_ == AppScreen::Playing || screen_ == AppScreen::Paused || screen_ == AppScreen::Death ||
        (screen_ == AppScreen::Settings && settingsReturn_ == AppScreen::Paused)) {
        nu::DrawItem player;
        player.mesh = &playerMesh_;
        player.transform = glm::translate(nu::Mat4{1.0f}, character().position());
        player.material.baseColor = ultActive() ? nu::Vec4{0.95f, 0.45f, 0.18f, 1.0f}
                                                : nu::Vec4{0.72f, 0.28f, 0.22f, 1.0f};
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
    }

    renderer().endFrame([this](VkCommandBuffer cmd) { imgui().endFrame(cmd); });
}

} // namespace nine
