#pragma once

#include "nu/math/math.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace nine {

enum class MapObjectKind {
    Imp,
    InkPool,
    Ruin
};

struct MapObject {
    MapObjectKind kind = MapObjectKind::Imp;
    nu::Vec3 position{};
    float aggroRadius = 16.0f;
    float radius = 1.5f;
};

struct ArenaMap {
    nu::Vec3 spawnPoint{0.0f, 2.0f, 0.0f};
    nu::Vec3 nextLevelPoint{0.0f, 0.05f, -36.0f};
    std::vector<MapObject> objects{};
};

[[nodiscard]] ArenaMap makeDefaultArenaMap();
[[nodiscard]] bool loadArenaMap(const std::filesystem::path& path, ArenaMap& outMap);
[[nodiscard]] bool saveArenaMap(const std::filesystem::path& path, const ArenaMap& map);
[[nodiscard]] const char* mapObjectKindName(MapObjectKind kind);

} // namespace nine
