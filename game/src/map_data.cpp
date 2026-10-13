#include "map_data.hpp"

#include "nu/core/log.hpp"

#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace nine {
namespace {

void writeVec3(std::ostream& os, const nu::Vec3& v) {
    os << '[' << v.x << ", " << v.y << ", " << v.z << ']';
}

bool parseVec3(const std::string& s, size_t& i, nu::Vec3& out) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) {
        ++i;
    }
    if (i >= s.size() || s[i] != '[') {
        return false;
    }
    ++i;
    float vals[3]{};
    for (int n = 0; n < 3; ++n) {
        while (i < s.size() && (s[i] == ' ' || s[i] == ',')) {
            ++i;
        }
        size_t end = i;
        while (end < s.size() && (s[end] == '-' || s[end] == '+' || s[end] == '.' || (s[end] >= '0' && s[end] <= '9') ||
                                  s[end] == 'e' || s[end] == 'E')) {
            ++end;
        }
        if (end == i) {
            return false;
        }
        vals[n] = std::strtof(s.c_str() + static_cast<long>(i), nullptr);
        i = end;
    }
    while (i < s.size() && s[i] != ']') {
        ++i;
    }
    if (i >= s.size() || s[i] != ']') {
        return false;
    }
    ++i;
    out = {vals[0], vals[1], vals[2]};
    return true;
}

} // namespace

ArenaMap makeDefaultArenaMap() {
    ArenaMap map;
    map.spawnPoint = {0.0f, 2.0f, 0.0f};
    map.nextLevelPoint = {0.0f, 0.05f, -36.0f};
    map.objects = {
        {MapObjectKind::Imp, {18.0f, 0.0f, 14.0f}, 16.0f, 1.5f},
        {MapObjectKind::InkPool, {4.0f, 0.05f, 4.0f}, 16.0f, 1.5f},
        {MapObjectKind::InkPool, {-10.0f, 0.05f, 2.0f}, 16.0f, 1.5f},
        {MapObjectKind::InkPool, {8.0f, 0.05f, -8.0f}, 16.0f, 1.5f},
        {MapObjectKind::Ruin, {-8.0f, 0.5f, 6.0f}, 16.0f, 1.0f},
        {MapObjectKind::Ruin, {10.0f, 0.5f, -4.0f}, 16.0f, 1.0f},
        {MapObjectKind::Ruin, {-14.0f, 0.5f, -10.0f}, 16.0f, 1.0f},
        {MapObjectKind::Ruin, {6.0f, 0.5f, 12.0f}, 16.0f, 1.0f},
        {MapObjectKind::Ruin, {-3.0f, 0.5f, -16.0f}, 16.0f, 1.0f},
        {MapObjectKind::Ruin, {16.0f, 0.5f, 8.0f}, 16.0f, 1.0f},
    };
    return map;
}

const char* mapObjectKindName(MapObjectKind kind) {
    switch (kind) {
    case MapObjectKind::Imp:
        return "imp";
    case MapObjectKind::InkPool:
        return "inkPool";
    case MapObjectKind::Ruin:
        return "ruin";
    }
    return "imp";
}

bool saveArenaMap(const std::filesystem::path& path, const ArenaMap& map) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        NU_LOG_WARN("Failed to save map {}", path.string());
        return false;
    }

    out << "{\n  \"spawn\": ";
    writeVec3(out, map.spawnPoint);
    out << ",\n  \"nextLevel\": ";
    writeVec3(out, map.nextLevelPoint);
    out << ",\n  \"objects\": [\n";
    for (size_t i = 0; i < map.objects.size(); ++i) {
        const MapObject& o = map.objects[i];
        out << "    {\"type\": \"" << mapObjectKindName(o.kind) << "\", \"pos\": ";
        writeVec3(out, o.position);
        out << ", \"aggroRadius\": " << o.aggroRadius << ", \"radius\": " << o.radius << '}';
        if (i + 1 < map.objects.size()) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n}\n";
    NU_LOG_INFO("Saved arena map {}", path.string());
    return true;
}

bool loadArenaMap(const std::filesystem::path& path, ArenaMap& outMap) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string text = ss.str();

    ArenaMap map = makeDefaultArenaMap();
    map.objects.clear();

    auto findKey = [&](const char* key) -> size_t {
        const std::string needle = std::string("\"") + key + "\"";
        const size_t at = text.find(needle);
        if (at == std::string::npos) {
            return std::string::npos;
        }
        size_t i = at + needle.size();
        while (i < text.size() && text[i] != ':') {
            ++i;
        }
        return i < text.size() ? i + 1 : std::string::npos;
    };

    if (size_t i = findKey("spawn"); i != std::string::npos) {
        parseVec3(text, i, map.spawnPoint);
    }
    if (size_t i = findKey("nextLevel"); i != std::string::npos) {
        parseVec3(text, i, map.nextLevelPoint);
    }

    const size_t objectsKey = text.find("\"objects\"");
    if (objectsKey != std::string::npos) {
        size_t i = text.find('[', objectsKey);
        if (i != std::string::npos) {
            ++i;
            while (i < text.size()) {
                while (i < text.size() && text[i] != '{' && text[i] != ']') {
                    ++i;
                }
                if (i >= text.size() || text[i] == ']') {
                    break;
                }
                const size_t objEnd = text.find('}', i);
                if (objEnd == std::string::npos) {
                    break;
                }
                const std::string chunk = text.substr(i, objEnd - i + 1);
                MapObject obj{};
                if (chunk.find("inkPool") != std::string::npos) {
                    obj.kind = MapObjectKind::InkPool;
                } else if (chunk.find("ruin") != std::string::npos) {
                    obj.kind = MapObjectKind::Ruin;
                } else {
                    obj.kind = MapObjectKind::Imp;
                }
                size_t local = 0;
                const size_t posKey = chunk.find("\"pos\"");
                if (posKey != std::string::npos) {
                    local = posKey;
                    while (local < chunk.size() && chunk[local] != '[') {
                        ++local;
                    }
                    parseVec3(chunk, local, obj.position);
                }
                auto readFloatAfter = [&](const char* key, float& dst) {
                    const size_t k = chunk.find(key);
                    if (k == std::string::npos) {
                        return;
                    }
                    size_t p = k;
                    while (p < chunk.size() && chunk[p] != ':') {
                        ++p;
                    }
                    if (p < chunk.size()) {
                        dst = std::strtof(chunk.c_str() + static_cast<long>(p + 1), nullptr);
                    }
                };
                readFloatAfter("\"aggroRadius\"", obj.aggroRadius);
                readFloatAfter("\"radius\"", obj.radius);
                map.objects.push_back(obj);
                i = objEnd + 1;
            }
        }
    }

    outMap = std::move(map);
    NU_LOG_INFO("Loaded arena map {} ({} objects)", path.string(), outMap.objects.size());
    return true;
}

} // namespace nine
