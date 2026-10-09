#include "nu/core/paths.hpp"

#include "nu/core/log.hpp"

namespace nu {

fs::path Paths::exeDir_;
fs::path Paths::dataDir_;
fs::path Paths::shaderDir_;

void Paths::init(const fs::path& exeDir) {
    exeDir_ = exeDir;
    dataDir_ = exeDir_ / "data";
    shaderDir_ = exeDir_ / "shaders";

    // Dev fallback: source tree next to the repo when staged folders are missing.
#ifdef NU_SHADER_DIR
    if (!fs::exists(shaderDir_)) {
        shaderDir_ = fs::path(NU_SHADER_DIR);
    }
#endif
#ifdef NU_ASSET_DIR
    if (!fs::exists(dataDir_)) {
        dataDir_ = fs::path(NU_ASSET_DIR);
    }
#endif

    NU_LOG_INFO("Paths: exe={}, data={}, shaders={}", exeDir_.string(), dataDir_.string(), shaderDir_.string());
}

fs::path Paths::exeDir() {
    return exeDir_;
}

fs::path Paths::dataDir() {
    return dataDir_;
}

fs::path Paths::shaderDir() {
    return shaderDir_;
}

fs::path Paths::resolveData(const std::string& relative) {
    return dataDir_ / relative;
}

fs::path Paths::resolveShader(const std::string& relative) {
    return shaderDir_ / relative;
}

} // namespace nu
