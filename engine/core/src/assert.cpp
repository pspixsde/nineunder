#include "nu/core/assert.hpp"

namespace nu::detail {

[[noreturn]] void assertFail(const char* expr, const char* file, int line, const char* msg) {
    NU_LOG_CRITICAL("ASSERT FAILED: {} at {}:{} — {}", expr, file, line, msg && msg[0] ? msg : "(no message)");
    Log::shutdown();
    std::abort();
}

} // namespace nu::detail
