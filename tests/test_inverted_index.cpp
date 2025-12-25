#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include "core/inverted_index.h"

TEST_CASE("Inverted index test")
{
    std::cout << "Running test_inverted_index\n";
    InvertedIndex idx;
    idx.add_document(1, "hello world");
    REQUIRE(idx.search("hello").size() == 1);
}
