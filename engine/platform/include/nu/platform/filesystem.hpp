#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace nu {

namespace fs = std::filesystem;

[[nodiscard]] std::optional<std::vector<uint8_t>> readBinaryFile(const fs::path& path);
[[nodiscard]] std::optional<std::string> readTextFile(const fs::path& path);
[[nodiscard]] bool writeBinaryFile(const fs::path& path, const void* data, size_t size);

[[nodiscard]] uint64_t fileWriteTimeNanos(const fs::path& path);

} // namespace nu
