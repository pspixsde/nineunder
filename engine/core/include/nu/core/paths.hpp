#pragma once

#include <filesystem>
#include <string>

namespace nu {

namespace fs = std::filesystem;

/// Resolves runtime data paths (exe-relative first, then source tree for dev).
class Paths {
public:
    static void init(const fs::path& exeDir);

    [[nodiscard]] static fs::path exeDir();
    [[nodiscard]] static fs::path dataDir();
    [[nodiscard]] static fs::path shaderDir();
    [[nodiscard]] static fs::path resolveData(const std::string& relative);
    [[nodiscard]] static fs::path resolveShader(const std::string& relative);

private:
    static fs::path exeDir_;
    static fs::path dataDir_;
    static fs::path shaderDir_;
};

} // namespace nu
