#include "nu/core/log.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <vector>

namespace nu {

std::shared_ptr<spdlog::logger> Log::engineLogger_;
std::shared_ptr<spdlog::logger> Log::gameLogger_;

void Log::init(std::string_view appName) {
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    try {
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("nine_under.log", true));
    } catch (const spdlog::spdlog_ex&) {
        // File sink optional (read-only cwd, etc.)
    }

    engineLogger_ = std::make_shared<spdlog::logger>("engine", sinks.begin(), sinks.end());
    gameLogger_ = std::make_shared<spdlog::logger>("game", sinks.begin(), sinks.end());

    engineLogger_->set_level(spdlog::level::trace);
    gameLogger_->set_level(spdlog::level::trace);
    engineLogger_->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");
    gameLogger_->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] %v");

    spdlog::register_logger(engineLogger_);
    spdlog::register_logger(gameLogger_);
    spdlog::set_default_logger(engineLogger_);

    engineLogger_->info("Logging initialized ({})", appName);
}

void Log::shutdown() {
    if (engineLogger_) {
        engineLogger_->info("Logging shutdown");
    }
    engineLogger_.reset();
    gameLogger_.reset();
    spdlog::shutdown();
}

spdlog::logger& Log::engine() {
    if (!engineLogger_) {
        init();
    }
    return *engineLogger_;
}

spdlog::logger& Log::game() {
    if (!gameLogger_) {
        init();
    }
    return *gameLogger_;
}

} // namespace nu
