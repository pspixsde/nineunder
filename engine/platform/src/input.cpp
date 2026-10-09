#include "nu/platform/input.hpp"

#include "nu/core/log.hpp"

#include <algorithm>
#include <cmath>

namespace nu {

Input::Input() {
    int count = 0;
    const SDL_JoystickID* pads = SDL_GetGamepads(&count);
    if (pads && count > 0) {
        gamepad_ = SDL_OpenGamepad(pads[0]);
        if (gamepad_) {
            NU_LOG_INFO("Gamepad connected: {}", SDL_GetGamepadName(gamepad_));
        }
    }
}

void Input::beginFrame() {
    keysPrev_ = keys_;
    mousePrev_ = mouse_;
    mouseDelta_ = Vec2{0.0f, 0.0f};
    scrollDelta_ = 0.0f;
}

void Input::handleEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        const Key key = mapScancode(event.key.scancode);
        const int idx = keyIndex(key);
        if (idx > 0) {
            keys_[static_cast<size_t>(idx)] = event.type == SDL_EVENT_KEY_DOWN;
        }
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        MouseButton btn = MouseButton::Count;
        if (event.button.button == SDL_BUTTON_LEFT) {
            btn = MouseButton::Left;
        } else if (event.button.button == SDL_BUTTON_RIGHT) {
            btn = MouseButton::Right;
        } else if (event.button.button == SDL_BUTTON_MIDDLE) {
            btn = MouseButton::Middle;
        }
        if (btn != MouseButton::Count) {
            mouse_[static_cast<size_t>(btn)] = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        }
        break;
    }
    case SDL_EVENT_MOUSE_MOTION:
        mousePos_ = Vec2{event.motion.x, event.motion.y};
        mouseDelta_ += Vec2{event.motion.xrel, event.motion.yrel};
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        scrollDelta_ += event.wheel.y;
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        if (!gamepad_) {
            gamepad_ = SDL_OpenGamepad(event.gdevice.which);
        }
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        if (gamepad_ && SDL_GetGamepadID(gamepad_) == event.gdevice.which) {
            SDL_CloseGamepad(gamepad_);
            gamepad_ = nullptr;
        }
        break;
    default:
        break;
    }
}

bool Input::keyDown(Key key) const {
    return keys_[static_cast<size_t>(keyIndex(key))];
}

bool Input::keyPressed(Key key) const {
    const int i = keyIndex(key);
    return keys_[static_cast<size_t>(i)] && !keysPrev_[static_cast<size_t>(i)];
}

bool Input::keyReleased(Key key) const {
    const int i = keyIndex(key);
    return !keys_[static_cast<size_t>(i)] && keysPrev_[static_cast<size_t>(i)];
}

bool Input::mouseDown(MouseButton button) const {
    return mouse_[static_cast<size_t>(button)];
}

bool Input::mousePressed(MouseButton button) const {
    const auto i = static_cast<size_t>(button);
    return mouse_[i] && !mousePrev_[i];
}

Vec2 Input::moveAxis() const {
    Vec2 axis{0.0f, 0.0f};
    if (keyDown(Key::W)) {
        axis.y += 1.0f;
    }
    if (keyDown(Key::S)) {
        axis.y -= 1.0f;
    }
    if (keyDown(Key::A)) {
        axis.x -= 1.0f;
    }
    if (keyDown(Key::D)) {
        axis.x += 1.0f;
    }

    if (gamepad_) {
        const float lx = SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTX) / 32767.0f;
        const float ly = SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTY) / 32767.0f;
        constexpr float dead = 0.2f;
        if (std::fabs(lx) > dead) {
            axis.x += lx;
        }
        if (std::fabs(ly) > dead) {
            axis.y -= ly; // SDL Y+ is down
        }
    }

    const float len = glm::length(axis);
    if (len > 1.0f) {
        axis /= len;
    }
    return axis;
}

Vec2 Input::lookAxis() const {
    Vec2 axis = mouseDelta_ * 0.12f;
    if (gamepad_) {
        const float rx = SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTX) / 32767.0f;
        const float ry = SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTY) / 32767.0f;
        constexpr float dead = 0.15f;
        if (std::fabs(rx) > dead) {
            axis.x += rx * 6.0f;
        }
        if (std::fabs(ry) > dead) {
            axis.y += ry * 6.0f;
        }
    }
    return axis;
}

void Input::setRelativeMouse(bool enabled) {
    relativeMouse_ = enabled;
    SDL_SetWindowRelativeMouseMode(SDL_GetMouseFocus(), enabled);
}

int Input::keyIndex(Key key) {
    return static_cast<int>(key);
}

Key Input::mapScancode(SDL_Scancode sc) {
    switch (sc) {
    case SDL_SCANCODE_W:
        return Key::W;
    case SDL_SCANCODE_A:
        return Key::A;
    case SDL_SCANCODE_S:
        return Key::S;
    case SDL_SCANCODE_D:
        return Key::D;
    case SDL_SCANCODE_Q:
        return Key::Q;
    case SDL_SCANCODE_E:
        return Key::E;
    case SDL_SCANCODE_SPACE:
        return Key::Space;
    case SDL_SCANCODE_LSHIFT:
        return Key::LeftShift;
    case SDL_SCANCODE_LCTRL:
        return Key::LeftCtrl;
    case SDL_SCANCODE_ESCAPE:
        return Key::Escape;
    case SDL_SCANCODE_F1:
        return Key::F1;
    case SDL_SCANCODE_F2:
        return Key::F2;
    case SDL_SCANCODE_TAB:
        return Key::Tab;
    default:
        return Key::Unknown;
    }
}

} // namespace nu
