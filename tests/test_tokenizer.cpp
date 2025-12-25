#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include "core/tokenizer.h"

TEST_CASE("Tokenizer test")
{
    std::cout << "Running test_tokenizer\n";
    auto tokens = tokenize("Hello World");
    REQUIRE(tokens.size() == 2);
}
