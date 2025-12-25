#include <catch2/catch_test_macros.hpp>
#include "core/query_parser.h"

TEST_CASE("Query parser splits terms and handles NOT")
{
    auto q = parse_query("hello -world");
    REQUIRE(q.terms == std::vector<std::string>{"hello"});
    REQUIRE(q.not_terms == std::vector<std::string>{"world"});
}

TEST_CASE("Query parser detects OR")
{
    auto q = parse_query("hello OR world");
    REQUIRE(q.is_or == true);
    REQUIRE(q.terms == std::vector<std::string>{"hello", "world"});
}
