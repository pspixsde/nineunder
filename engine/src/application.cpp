#include "nu/application.hpp"

#include "nu/core/log.hpp"
#include "nu/core/paths.hpp"

#include <SDL3/SDL.h>

#ifdef TRACY_ENABLE
#    include <tracy/Tracy.hpp>
#endif

#include <imgui.h>

#include <filesystem>
#include <stdexcept>

namespace nu {

Application::Application(ApplicationDesc desc) : desc_(std::move(desc)) {}

Application::~Application() {
    shutdownSystems();
}

void Application::initSystems() {
    Log::init(desc_.title);
    NU_LOG_INFO("Nine Under engine {}.{}.{}", NU_VERSION_MAJOR, NU_VERSION_MINOR, NU_VERSION_PATCH);

    WindowDesc winDesc;
    winDesc.title = desc_.title;
    winDesc.width = desc_.windowWidth;
    winDesc.height = desc_.windowHeight;
    winDesc.fullscreen = desc_.fullscreen;
    window_ = std::make_unique<Window>(winDesc);
    input_ = std::make_unique<Input>();

    const char* basePath = SDL_GetBasePath();
    const std::filesystem::path exeDir =
        basePath ? std::filesystem::path(basePath) : std::filesystem::current_path();
    // SDL3: SDL_GetBasePath() returns an internal string; do not free.
    Paths::init(exeDir);

    renderer_ = std::make_unique<Renderer>();
    renderer_->init(window_->sdlWindow(), static_cast<uint32_t>(window_->width()),
                    static_cast<uint32_t>(window_->height()));

    imgui_ = std::make_unique<ImGuiLayer>();
    imgui_->init(window_->sdlWindow(), *renderer_);

    audio_ = std::make_unique<AudioEngine>();
    audio_->init();

    physics_ = std::make_unique<PhysicsWorld>();
    physics_->init();

    character_ = std::make_unique<CharacterController>();
    CharacterDesc characterDesc;
    characterDesc.initialPosition = {0.0f, 2.0f, 0.0f};
    character_->init(*physics_, characterDesc);

    scene_ = std::make_unique<SceneWorld>();

    camera_.setPerspective(60.0f, window_->aspect(), 0.1f, 400.0f);
    camera_.setMode(CameraMode::ThirdPerson);
    camera_.yaw = -90.0f;
    camera_.pitch = -25.0f;
    input_->setRelativeMouse(false);

    clock_.reset();
    onInit();
}

void Application::shutdownSystems() {
    onShutdown();
    if (imgui_) {
        imgui_->shutdown();
        imgui_.reset();
    }
    if (audio_) {
        audio_->shutdown();
        audio_.reset();
    }
    if (character_) {
        character_->shutdown();
        character_.reset();
    }
    if (physics_) {
        physics_->shutdown();
        physics_.reset();
    }
    if (renderer_) {
        renderer_->shutdown();
        renderer_.reset();
    }
    scene_.reset();
    input_.reset();
    window_.reset();
    Log::shutdown();
}

void Application::processEvents() {
    input_->beginFrame();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        imgui_->processEvent(event);
        input_->handleEvent(event);

        if (event.type == SDL_EVENT_QUIT) {
            running_ = false;
        } else if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
            window_->onResize(event.window.data1, event.window.data2);
            renderer_->resize(static_cast<uint32_t>(window_->width()), static_cast<uint32_t>(window_->height()));
            camera_.setAspect(window_->aspect());
        }
    }

    if (input_->keyPressed(Key::Escape)) {
        onEscapePressed();
    }
    if (gameplayActive_ && input_->keyPressed(Key::F1)) {
        freeCameraMode_ = !freeCameraMode_;
        camera_.setMode(freeCameraMode_ ? CameraMode::Free : CameraMode::ThirdPerson);
        NU_LOG_INFO("Camera mode: {}", freeCameraMode_ ? "Free" : "ThirdPerson");
    }
    if (gameplayActive_ && input_->keyPressed(Key::Tab)) {
        input_->setRelativeMouse(!input_->relativeMouse());
    }
}

void Application::onEscapePressed() {
    requestQuit();
}

void Application::onFixedUpdate(float dt) {
    physics_->update(dt);

    if (!gameplayActive_) {
        camera_.update();
        return;
    }

    const Vec2 move = input_->moveAxis();
    const Vec2 look = input_->lookAxis();

    if (freeCameraMode_) {
        camera_.addYawPitch(look.x, -look.y);
        Vec3 local{0.0f, 0.0f, 0.0f};
        local.z += move.y;
        local.x += move.x;
        if (input_->keyDown(Key::E)) {
            local.y += 1.0f;
        }
        if (input_->keyDown(Key::Q)) {
            local.y -= 1.0f;
        }
        const float speed = input_->keyDown(Key::LeftShift) ? 24.0f : 10.0f;
        camera_.moveLocal(local * speed * dt);
    } else {
        camera_.addOrbit(look.x, -look.y);
        camera_.addDistance(-input_->scrollDelta() * 0.75f);

        const Vec3 forward = camera_.forward();
        const Vec3 flatForward = glm::normalize(Vec3{forward.x, 0.0f, forward.z});
        const Vec3 flatRight = glm::normalize(glm::cross(flatForward, Vec3{0, 1, 0}));
        const Vec3 wish = flatRight * move.x + flatForward * move.y;
        // Jump removed for Nine Under (grounded melee fantasy).
        character_->update(dt, wish, false);
        camera_.setTarget(character_->position());
    }

    camera_.update();
}

void Application::onVariableUpdate(float) {
    if (audio_) {
        audio_->update();
    }
}

void Application::onRender(float) {
    RenderScene scene;
    // Subclasses / game fill draws; default shows nothing extra.
    renderer_->beginFrame();
    renderer_->drawScene(camera_, scene);
    imgui_->beginFrame();

    ImGui::Begin("Nine Under");
    ImGui::Text("FPS: %.1f", clock_.fps());
    ImGui::Text("Camera: %s", freeCameraMode_ ? "Free (F1)" : "ThirdPerson (F1)");
    const Vec3 p = character_->position();
    ImGui::Text("Player: %.2f %.2f %.2f", p.x, p.y, p.z);
    ImGui::Text("On ground: %s", character_->onGround() ? "yes" : "no");
    ImGui::TextUnformatted("WASD move | Mouse look (no jump)");
    ImGui::End();

    renderer_->endFrame([this](VkCommandBuffer cmd) { imgui_->endFrame(cmd); });
}

int Application::run() {
    try {
        initSystems();
    } catch (const std::exception& ex) {
        NU_LOG_CRITICAL("Init failed: {}", ex.what());
        shutdownSystems();
        return 1;
    }

    running_ = true;
    float accumulator = 0.0f;

    while (running_ && !window_->shouldClose()) {
#ifdef TRACY_ENABLE
        FrameMark;
#endif
        processEvents();
        const float frameDt = clock_.tick();
        accumulator += frameDt;

        int steps = 0;
        while (accumulator >= kFixedDt && steps < 8) {
            onFixedUpdate(kFixedDt);
            accumulator -= kFixedDt;
            ++steps;
        }

        onVariableUpdate(frameDt);
        const float alpha = accumulator / kFixedDt;
        onRender(alpha);
    }

    shutdownSystems();
    return 0;
}

} // namespace nu
