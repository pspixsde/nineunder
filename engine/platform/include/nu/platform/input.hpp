#pragma once

#include "nu/math/math.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>

namespace nu {

enum class Key : uint16_t {
    Unknown = 0,
    W,
    A,
    S,
    D,
    Q,
    E,
    R,
    F,
    Space,
    LeftShift,
    LeftCtrl,
    Escape,
    F1,
    F2,
    Tab,
    Count
};

enum class MouseButton : uint8_t {
    Left = 0,
    Right,
    Middle,
    Count
};

class Input {
public:
    Input();

    void beginFrame();
    void handleEvent(const SDL_Event& event);

    [[nodiscard]] bool keyDown(Key key) const;
    /// Rising edge latched until `clearPressedLatches()` (survives frames with no fixed step).
    [[nodiscard]] bool keyPressed(Key key) const;
    /// Same as keyPressed, but clears the latch for that key.
    [[nodiscard]] bool consumeKeyPressed(Key key);
    [[nodiscard]] bool keyReleased(Key key) const;
    void clearPressedLatches();

    [[nodiscard]] bool mouseDown(MouseButton button) const;
    [[nodiscard]] bool mousePressed(MouseButton button) const;

    [[nodiscard]] Vec2 mousePosition() const { return mousePos_; }
    [[nodiscard]] Vec2 mouseDelta() const { return mouseDelta_; }
    [[nodiscard]] float scrollDelta() const { return scrollDelta_; }

    [[nodiscard]] Vec2 moveAxis() const;   // WASD / left stick
    [[nodiscard]] Vec2 lookAxis() const;   // mouse delta / right stick

    void setRelativeMouse(bool enabled);
    [[nodiscard]] bool relativeMouse() const { return relativeMouse_; }

    /// Multiplier on mouse look (1.0 = default).
    void setLookSensitivity(float sensitivity);
    [[nodiscard]] float lookSensitivity() const { return lookSensitivity_; }

private:
    static int keyIndex(Key key);
    static Key mapScancode(SDL_Scancode sc);

    std::array<bool, static_cast<size_t>(Key::Count)> keys_{};
    std::array<bool, static_cast<size_t>(Key::Count)> keysPrev_{};
    std::array<bool, static_cast<size_t>(Key::Count)> pressedLatch_{};
    std::array<bool, static_cast<size_t>(MouseButton::Count)> mouse_{};
    std::array<bool, static_cast<size_t>(MouseButton::Count)> mousePrev_{};

    Vec2 mousePos_{};
    Vec2 mouseDelta_{};
    float scrollDelta_ = 0.0f;
    bool relativeMouse_ = false;
    float lookSensitivity_ = 1.0f;

    SDL_Gamepad* gamepad_ = nullptr;
};

} // namespace nu
