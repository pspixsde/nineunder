#pragma once

#include <entt/entt.hpp>

namespace nu {

using Entity = entt::entity;

class SceneWorld {
public:
    [[nodiscard]] entt::registry& registry() { return registry_; }
    [[nodiscard]] const entt::registry& registry() const { return registry_; }

    Entity create() { return registry_.create(); }
    void destroy(Entity e) { registry_.destroy(e); }

    template <typename T, typename... Args>
    T& emplace(Entity e, Args&&... args) {
        return registry_.emplace<T>(e, std::forward<Args>(args)...);
    }

    template <typename T>
    T* tryGet(Entity e) {
        return registry_.try_get<T>(e);
    }

    void clear() { registry_.clear(); }

private:
    entt::registry registry_;
};

} // namespace nu
