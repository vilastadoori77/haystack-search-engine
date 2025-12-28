#include <catch2/catch_test_macros.hpp>
#include "core/search_service.h"
#include <cctype>
#include <string>

static std::string lower(std::string s)
{
    for (auto &c : s)
        c = (char)std::tolower((unsigned char)c);
    return s;
}

TEST_CASE("Search returns a snippet containing query terms")
{
    SearchService s;
    s.add_document(1, "Teamcenter migration guide: map attributes , validate schema, run dry-run.");
    auto r = s.search_with_snippets("migration schema");

    REQUIRE(r.size() == 1);
    REQUIRE(r[0].docId == 1);

    auto snip = lower(r[0].snippet);
    REQUIRE(snip.find("migration") != std::string::npos);
    REQUIRE(snip.find("schema") != std::string::npos);
}