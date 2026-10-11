#include "game_app.hpp"

#include "arena_greybox.hpp"

#include "nu/core/log.hpp"
#include "nu/core/paths.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>

namespace nine {
namespace {

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
    outScreen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * fbHeight;
    return true;
}

} // namespace

GameApp::GameApp() : nu::Application(nu::ApplicationDesc{"Nine Under", 1600, 900, false}) {}

void GameApp::onInit() {
    NU_GAME_INFO("Game init — Arena I (The Grey Threshold) v0.1.1");
    buildArenaOne();
    tryLoadUrns();
    setupHotReload();
    respawnImp();
    enterMainMenu();
}

void GameApp::onShutdown() {
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
    meshesReady_ = false;
}

void GameApp::buildArenaOne() {
    auto& factory = renderer().meshes();
    floorMesh_ = factory.createPlane(80.0f, 8);
    cubeMesh_ = factory.createCube(1.0f);
    wallMesh_ = factory.createCube(1.0f);
    playerMesh_ = factory.createCapsule(0.4f, 0.5f, 10);
    impMesh_ = factory.createCapsule(0.35f, 0.45f, 10);
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
        // Fallback collider if the GLB has no COL_* nodes
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
            if (urnProp_) {
                urnProp_->destroy(renderer());
                urnProp_.reset();
            }
            tryLoadUrns();
        }
    });
}

void GameApp::enterMainMenu() {
    screen_ = AppScreen::MainMenu;
    gameplayActive_ = false;
    freeCameraMode_ = false;
    camera().setMode(nu::CameraMode::ThirdPerson);
    input().setRelativeMouse(false);
}

void GameApp::startPlay() {
    screen_ = AppScreen::Playing;
    gameplayActive_ = true;
    input().setRelativeMouse(true);
}

void GameApp::respawnImp() {
    imp_.position = {5.0f, 0.0f, 5.0f};
    imp_.maxHp = 360.0f;
    imp_.hp = imp_.maxHp;
    imp_.alive = true;
}

void GameApp::killImp() {
    if (!imp_.alive) {
        return;
    }
    imp_.hp = 0.0f;
    imp_.alive = false;

    std::uniform_real_distribution<float> pitchDist(0.88f, 1.12f);
    const float pitch = pitchDist(rng_);
    const auto sfx = nu::Paths::resolveData("audio/sfx/imp_death.wav");
    audio().playSound(sfx, pitch);
    NU_GAME_INFO("Imp died (death SFX pitch={:.2f})", pitch);
}

void GameApp::onEscapePressed() {
    if (screen_ == AppScreen::Playing) {
        enterMainMenu();
        return;
    }
    requestQuit();
}

void GameApp::onFixedUpdate(float dt) {
    Application::onFixedUpdate(dt);
}

void GameApp::onVariableUpdate(float dt) {
    Application::onVariableUpdate(dt);
    hotReload_.poll();

    if (screen_ != AppScreen::Playing) {
        return;
    }

    // Temporary: F2 kills the Imp (or respawns if already dead) for audio testing.
    if (input().keyPressed(nu::Key::F2)) {
        if (imp_.alive) {
            killImp();
        } else {
            respawnImp();
            NU_GAME_INFO("Imp respawned (F2)");
        }
    }
    (void)dt;
}

void GameApp::drawMainMenu() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2{0.5f, 0.5f});
    ImGui::SetNextWindowSize(ImVec2{360.0f, 220.0f}, ImGuiCond_Always);
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
    ImGui::Dummy(ImVec2{0.0f, 24.0f});

    const float btnW = 220.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Play", ImVec2{btnW, 40.0f})) {
        startPlay();
    }
    ImGui::Dummy(ImVec2{0.0f, 10.0f});
    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - btnW) * 0.5f);
    if (ImGui::Button("Quit", ImVec2{btnW, 40.0f})) {
        requestQuit();
    }

    ImGui::End();
}

void GameApp::drawImpHpBar() {
    if (!imp_.alive) {
        return;
    }

    const float fbW = static_cast<float>(window().width());
    const float fbH = static_cast<float>(window().height());
    nu::Vec2 screen{};
    const nu::Vec3 head = imp_.position + nu::Vec3{0.0f, 1.85f, 0.0f};
    if (!projectWorldToScreen(camera(), head, fbW, fbH, screen)) {
        return;
    }

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
}

void GameApp::drawGameplayHud() {
    ImGui::Begin("Nine Under — Arena I");
    ImGui::TextUnformatted("The Grey Threshold");
    ImGui::Separator();
    ImGui::TextUnformatted("WASD move | Mouse look");
    ImGui::TextUnformatted("F1 free cam | Tab capture mouse | Esc menu");
    ImGui::TextUnformatted("F2 kill/respawn Imp (temp audio test)");
    const nu::Vec3 p = character().position();
    ImGui::Text("Pos %.2f %.2f %.2f | grounded %s", p.x, p.y, p.z, character().onGround() ? "yes" : "no");
    ImGui::Text("Urns: %s (%zu)", urnProp_ ? "prop_ink_urn" : "(none)", urnTransforms_.size());
    ImGui::Text("Imp: %s (%.0f / %.0f HP)", imp_.alive ? "alive" : "dead", imp_.hp, imp_.maxHp);
    ImGui::End();

    drawImpHpBar();
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

    if (imp_.alive) {
        nu::DrawItem impDraw;
        impDraw.mesh = &impMesh_;
        impDraw.transform = glm::translate(nu::Mat4{1.0f}, imp_.position + nu::Vec3{0.0f, 0.9f, 0.0f});
        impDraw.material.baseColor = {0.55f, 0.18f, 0.22f, 1.0f};
        impDraw.material.roughness = 0.65f;
        impDraw.material.metallic = 0.05f;
        scene.draws.push_back(impDraw);
    }

    if (screen_ == AppScreen::Playing) {
        nu::DrawItem player;
        player.mesh = &playerMesh_;
        player.transform = glm::translate(nu::Mat4{1.0f}, character().position());
        player.material.baseColor = {0.72f, 0.28f, 0.22f, 1.0f};
        player.material.roughness = 0.7f;
        scene.draws.push_back(player);
    }

    renderer().beginFrame();
    renderer().drawScene(camera(), scene);

    imgui().beginFrame();
    if (screen_ == AppScreen::MainMenu) {
        drawMainMenu();
    } else {
        drawGameplayHud();
    }

    renderer().endFrame([this](VkCommandBuffer cmd) { imgui().endFrame(cmd); });
}

} // namespace nine
