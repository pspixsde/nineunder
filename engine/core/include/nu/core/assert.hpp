#pragma once

#include "nu/core/log.hpp"

#include <cstdlib>

namespace nu::detail {
[[noreturn]] void assertFail(const char* expr, const char* file, int line, const char* msg);
} // namespace nu::detail

#if defined(NU_ASSERTS)
#    define NU_ASSERT(expr)                                                                                            \
        do {                                                                                                           \
            if (!(expr)) {                                                                                             \
                ::nu::detail::assertFail(#expr, __FILE__, __LINE__, "");                                               \
            }                                                                                                          \
        } while (0)
#    define NU_ASSERT_MSG(expr, msg)                                                                                   \
        do {                                                                                                           \
            if (!(expr)) {                                                                                             \
                ::nu::detail::assertFail(#expr, __FILE__, __LINE__, msg);                                              \
            }                                                                                                          \
        } while (0)
#else
#    define NU_ASSERT(expr) ((void)0)
#    define NU_ASSERT_MSG(expr, msg) ((void)0)
#endif

#define NU_VERIFY(expr)                                                                                                \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            ::nu::detail::assertFail(#expr, __FILE__, __LINE__, "verify failed");                                      \
        }                                                                                                              \
    } while (0)
