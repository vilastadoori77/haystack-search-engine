#include <catch2/catch_test_macros.hpp>
#include "core/search_service.h"

TEST_CASE("Results are ranked by term frequency")
{
    SearchService ss;
    ss.add_document(1, "apple banana apple");
    ss.add_document(2, "banana cherry banana banana");
    ss.add_document(3, "cherry date cherry cherry cherry");

    auto results = ss.search("banana");
    REQUIRE(results == std::vector<int>{1, 2});
}