#include "nu/core/time.hpp"

#include <doctest/doctest.h>

TEST_CASE("fixed timestep constants") {
    CHECK(nu::kFixedHz == doctest::Approx(60.0f));
    CHECK(nu::kFixedDt == doctest::Approx(1.0f / 60.0f));
}
