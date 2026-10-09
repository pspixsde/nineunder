#pragma once

#include "nu/render/renderer.hpp"

struct SDL_Window;
union SDL_Event;

namespace nu {

class ImGuiLayer {
public:
    void init(SDL_Window* window, Renderer& renderer);
    void shutdown();

    void beginFrame();
    void endFrame(VkCommandBuffer cmd);
    void processEvent(const SDL_Event& event);

    [[nodiscard]] bool ready() const { return ready_; }

private:
    bool ready_ = false;
    Renderer* renderer_ = nullptr;
    VkDescriptorPool imguiPool_ = VK_NULL_HANDLE;
};

} // namespace nu
