#include "nu/assets/asset_registry.hpp"
#include "nu/core/types.hpp"

#include <doctest/doctest.h>
#include <string>

TEST_CASE("generational handles stay unique after recycle") {
    nu::AssetRegistry<std::string, nu::MeshHandle> registry;

    const nu::MeshHandle a = registry.add(std::string{"mesh_a"}, "a");
    CHECK(a.valid());
    CHECK(registry.get(a) != nullptr);
    CHECK(*registry.get(a) == "mesh_a");

    registry.remove(a);
    CHECK(registry.get(a) == nullptr);

    const nu::MeshHandle b = registry.add(std::string{"mesh_b"}, "b");
    CHECK(b.index == a.index);
    CHECK(b.generation != a.generation);
    CHECK(registry.get(a) == nullptr);
    CHECK(registry.get(b) != nullptr);
}
