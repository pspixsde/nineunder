#include "nu/math/math.hpp"

#include <doctest/doctest.h>

TEST_CASE("lerp and perspective helpers") {
    CHECK(nu::lerp(0.0f, 10.0f, 0.5f) == doctest::Approx(5.0f));

    const nu::Mat4 p = nu::perspectiveVk(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    CHECK(p[0][0] != 0.0f);

    const nu::Vec3 a{0, 0, 0};
    const nu::Vec3 b{2, 4, 6};
    const nu::Vec3 m = nu::lerp(a, b, 0.5f);
    CHECK(m.x == doctest::Approx(1.0f));
    CHECK(m.y == doctest::Approx(2.0f));
    CHECK(m.z == doctest::Approx(3.0f));
}
