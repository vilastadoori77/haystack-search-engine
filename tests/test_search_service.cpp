#include <catch2/catch_test_macros.hpp>
#include "core/search_service.h"

TEST_CASE("SearchService applies NOT filter")
{
    SearchService ss;
    ss.add_document(1, "hello world");
    ss.add_document(2, "hello there");
    ss.add_document(3, "goodbye world");

    auto results = ss.search("hello -world");
    REQUIRE(results == std::vector<int>{2});
}

TEST_CASE("SearchService supports OR")
{
    SearchService ss;
    ss.add_document(1, "apple banana");
    ss.add_document(2, "banana cherry");
    ss.add_document(3, "cherry date");
    auto results = ss.search("apple OR date");
    REQUIRE(results == std::vector<int>{1, 3});
}