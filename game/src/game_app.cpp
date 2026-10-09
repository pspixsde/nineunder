#include "game_app.hpp"

#include "arena_greybox.hpp"

#include "nu/core/log.hpp"

#include <imgui.h>

namespace nine {

GameApp::GameApp() : nu::Application(nu::ApplicationDesc{"Nine Under", 1600, 900, false}) {}

void GameApp::onInit() {
    NU_GAME_INFO("Game init — Arena I graybox (The Grey Threshold)");
    buildArenaOne();
}

void GameApp::onShutdown() {
    if (!meshesReady_) {
        return;
    }
    renderer().meshes().destroy(floorMesh_);
    renderer().meshes().destroy(cubeMesh_);
    renderer().meshes().destroy(playerMesh_);
    renderer().meshes().destroy(wallMesh_);
    meshesReady_ = false;
}

void GameApp::buildArenaOne() {
    auto& factory = renderer().meshes();
    floorMesh_ = factory.createPlane(80.0f, 8);
    cubeMesh_ = factory.createCube(1.0f);
    wallMesh_ = factory.createCube(1.0f);
    playerMesh_ = factory.createCapsule(0.4f, 0.5f, 10);
    meshesReady_ = true;

    const ArenaGreybox arena =
        buildArenaOneGreybox(factory, physics(), floorMesh_, cubeMesh_, wallMesh_);
    staticDraws_ = arena.draws;

    // Physics ground + a few props already added inside buildArenaOneGreybox.
    NU_GAME_INFO("Arena I graybox ready ({} draw items)", staticDraws_.size());
}

void GameApp::onFixedUpdate(float dt) {
    Application::onFixedUpdate(dt);
}

void GameApp::onRender(float) {
    nu::RenderScene scene;
    scene.clearColor = {0.18f, 0.19f, 0.22f, 1.0f};
    scene.fogColor = {0.32f, 0.34f, 0.38f};
    scene.fogDensity = 0.028f;
    scene.lightDir = {-0.25f, -0.9f, -0.2f};
    scene.lightColor = {1.35f, 1.05f, 0.75f}; // stagnant dawn
    scene.ambientColor = {0.10f, 0.11f, 0.14f};
    scene.draws = staticDraws_;

    // Player capsule visual
    nu::DrawItem player;
    player.mesh = &playerMesh_;
    player.transform = glm::translate(nu::Mat4{1.0f}, character().position());
    player.material.baseColor = {0.72f, 0.28f, 0.22f, 1.0f};
    player.material.roughness = 0.7f;
    scene.draws.push_back(player);

    renderer().beginFrame();
    renderer().drawScene(camera(), scene);

    imgui().beginFrame();
    ImGui::Begin("Nine Under — Arena I");
    ImGui::TextUnformatted("The Grey Threshold");
    ImGui::Separator();
    ImGui::TextUnformatted("WASD move | Mouse look | Space jump");
    ImGui::TextUnformatted("F1 free cam | Tab capture mouse | Esc quit");
    const nu::Vec3 p = character().position();
    ImGui::Text("Pos %.2f %.2f %.2f | grounded %s", p.x, p.y, p.z, character().onGround() ? "yes" : "no");
    ImGui::End();

    renderer().endFrame([this](VkCommandBuffer cmd) { imgui().endFrame(cmd); });
}

} // namespace nine
