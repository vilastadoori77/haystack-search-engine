#include <catch2/catch_test_macros.hpp>
#include "core/search_service.h"

// Test case for the BM25 ranks shorter relevant doc higher than longer irrelevant doc
TEST_CASE("BM25 ranks shorter relevant doc higher than longer irrelevant doc")
{
    SearchService ss;
    ss.add_document(1, "hello filler filler filler filler filler filler filler filler filler filler world");
    ss.add_document(2, "hello world");

    auto results = ss.search("hello world");
    REQUIRE(results.front() == 2);
}