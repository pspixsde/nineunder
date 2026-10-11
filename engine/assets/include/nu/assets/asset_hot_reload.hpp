#pragma once

#include "nu/platform/filesystem.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace nu {

/// Polls watched files for write-time changes (dev builds).
class AssetHotReload {
public:
    using ReloadFn = std::function<void(const std::filesystem::path&)>;

    void watch(const std::filesystem::path& path, ReloadFn onChange);
    void watchDirectory(const std::filesystem::path& dir, const std::string& extension, ReloadFn onChange);
    void poll();

private:
    struct WatchEntry {
        std::filesystem::path path;
        uint64_t lastWrite = 0;
        ReloadFn onChange;
    };
    std::vector<WatchEntry> entries_;
};

} // namespace nu
