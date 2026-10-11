#include "nu/assets/asset_hot_reload.hpp"

#include "nu/core/log.hpp"

namespace nu {

void AssetHotReload::watch(const std::filesystem::path& path, ReloadFn onChange) {
    WatchEntry e;
    e.path = path;
    e.lastWrite = fileWriteTimeNanos(path);
    e.onChange = std::move(onChange);
    entries_.push_back(std::move(e));
}

void AssetHotReload::watchDirectory(const std::filesystem::path& dir, const std::string& extension,
                                    ReloadFn onChange) {
    if (!std::filesystem::exists(dir)) {
        return;
    }
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        if (!extension.empty() && entry.path().extension().string() != extension) {
            continue;
        }
        watch(entry.path(), onChange);
    }
}

void AssetHotReload::poll() {
    for (WatchEntry& e : entries_) {
        if (!std::filesystem::exists(e.path)) {
            continue;
        }
        const uint64_t w = fileWriteTimeNanos(e.path);
        if (w != 0 && w != e.lastWrite) {
            e.lastWrite = w;
            NU_LOG_INFO("Hot-reload: {}", e.path.string());
            if (e.onChange) {
                e.onChange(e.path);
            }
        }
    }
}

} // namespace nu
