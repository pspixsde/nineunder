#include "nu/platform/filesystem.hpp"

#include "nu/core/log.hpp"

#include <fstream>

namespace nu {

std::optional<std::vector<uint8_t>> readBinaryFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        NU_LOG_ERROR("Failed to open file: {}", path.string());
        return std::nullopt;
    }
    const auto size = file.tellg();
    if (size < 0) {
        return std::nullopt;
    }
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

std::optional<std::string> readTextFile(const fs::path& path) {
    auto bytes = readBinaryFile(path);
    if (!bytes) {
        return std::nullopt;
    }
    return std::string(bytes->begin(), bytes->end());
}

bool writeBinaryFile(const fs::path& path, const void* data, size_t size) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        NU_LOG_ERROR("Failed to write file: {}", path.string());
        return false;
    }
    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    return static_cast<bool>(file);
}

uint64_t fileWriteTimeNanos(const fs::path& path) {
    std::error_code ec;
    const auto ftime = fs::last_write_time(path, ec);
    if (ec) {
        return 0;
    }
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(ftime.time_since_epoch());
    return static_cast<uint64_t>(ns.count());
}

} // namespace nu
