#pragma once

#include <spdlog/spdlog.h>

#include <memory>
#include <string_view>

namespace nu {

class Log {
public:
    static void init(std::string_view appName = "NineUnder");
    static void shutdown();

    static spdlog::logger& engine();
    static spdlog::logger& game();

private:
    static std::shared_ptr<spdlog::logger> engineLogger_;
    static std::shared_ptr<spdlog::logger> gameLogger_;
};

} // namespace nu

#define NU_LOG_TRACE(...) ::nu::Log::engine().trace(__VA_ARGS__)
#define NU_LOG_DEBUG(...) ::nu::Log::engine().debug(__VA_ARGS__)
#define NU_LOG_INFO(...) ::nu::Log::engine().info(__VA_ARGS__)
#define NU_LOG_WARN(...) ::nu::Log::engine().warn(__VA_ARGS__)
#define NU_LOG_ERROR(...) ::nu::Log::engine().error(__VA_ARGS__)
#define NU_LOG_CRITICAL(...) ::nu::Log::engine().critical(__VA_ARGS__)

#define NU_GAME_TRACE(...) ::nu::Log::game().trace(__VA_ARGS__)
#define NU_GAME_DEBUG(...) ::nu::Log::game().debug(__VA_ARGS__)
#define NU_GAME_INFO(...) ::nu::Log::game().info(__VA_ARGS__)
#define NU_GAME_WARN(...) ::nu::Log::game().warn(__VA_ARGS__)
#define NU_GAME_ERROR(...) ::nu::Log::game().error(__VA_ARGS__)
