#pragma once

#include "nu/audio/audio_engine.hpp"
#include "nu/core/time.hpp"
#include "nu/platform/input.hpp"
#include "nu/platform/window.hpp"
#include "nu/render/camera.hpp"
#include "nu/render/renderer.hpp"
#include "nu/physics/physics_world.hpp"
#include "nu/physics/character_controller.hpp"
#include "nu/scene/world.hpp"
#include "nu/ui/imgui_layer.hpp"

#include <memory>
#include <string>

namespace nu {

struct ApplicationDesc {
    std::string title = "Nine Under";
    int windowWidth = 1600;
    int windowHeight = 900;
    bool fullscreen = false;
};

/// Owns the main loop: fixed 60 Hz sim + interpolated render.
class Application {
public:
    explicit Application(ApplicationDesc desc = {});
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    int run();

protected:
    virtual void onInit() {}
    virtual void onShutdown() {}
    virtual void onFixedUpdate(float dt);
    virtual void onVariableUpdate(float dt);
    virtual void onRender(float alpha);
    /// Default: quit. Override for main-menu / pause flows.
    virtual void onEscapePressed();

    void requestQuit() { running_ = false; }

    Window& window() { return *window_; }
    Input& input() { return *input_; }
    Renderer& renderer() { return *renderer_; }
    PhysicsWorld& physics() { return *physics_; }
    CharacterController& character() { return *character_; }
    SceneWorld& scene() { return *scene_; }
    Camera& camera() { return camera_; }
    ImGuiLayer& imgui() { return *imgui_; }
    AudioEngine& audio() { return *audio_; }
    [[nodiscard]] float fps() const { return clock_.fps(); }

    bool freeCameraMode_ = false;
    /// When false, character/camera gameplay input is skipped (e.g. main menu).
    bool gameplayActive_ = true;

private:
    void initSystems();
    void shutdownSystems();
    void processEvents();

    ApplicationDesc desc_;
    bool running_ = false;

    std::unique_ptr<Window> window_;
    std::unique_ptr<Input> input_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<PhysicsWorld> physics_;
    std::unique_ptr<CharacterController> character_;
    std::unique_ptr<SceneWorld> scene_;
    std::unique_ptr<ImGuiLayer> imgui_;
    std::unique_ptr<AudioEngine> audio_;
    Camera camera_;
    Clock clock_;
};

} // namespace nu
