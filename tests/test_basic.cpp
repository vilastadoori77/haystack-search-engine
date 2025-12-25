#include <catch2/catch_test_macros.hpp>
#include <iostream>

TEST_CASE("Basic sanity test")
{
    std::cout << "Running test_basic\n";
    REQUIRE(1 + 1 == 2);
}
