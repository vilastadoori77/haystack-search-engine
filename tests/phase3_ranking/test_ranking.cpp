#include "bm25_fixture.hpp"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <vector>

namespace
{

std::vector<int> doc_ids(const std::vector<std::pair<int, double>> &scored)
{
    std::vector<int> ids;
    ids.reserve(scored.size());
    for (const auto &p : scored)
    {
        ids.push_back(p.first);
    }
    return ids;
}

} // namespace

TEST_CASE("Phase 3: and_example alpha beta", "[phase3][ranking][3.0]")
{
    SearchService ss;
    ss.add_document(1, "alpha bravo");
    ss.add_document(2, "alpha charlie");
    ss.add_document(3, "bravo charlie");
    auto scored = ss.search_scored("alpha bravo");
    REQUIRE(doc_ids(scored) == std::vector<int>{1});
}

TEST_CASE("Phase 3: or_example alpha beta OR gamma global union", "[phase3][ranking][3.0]")
{
    SearchService ss;
    ss.add_document(1, "alpha bravo");
    ss.add_document(2, "bravo charlie");
    ss.add_document(3, "charlie gamma");
    ss.add_document(4, "delta epsilon");
    auto scored = ss.search_scored("alpha beta OR gamma");
    auto ids = doc_ids(scored);
    REQUIRE(std::find(ids.begin(), ids.end(), 1) != ids.end());
    REQUIRE(std::find(ids.begin(), ids.end(), 3) != ids.end());
    REQUIRE(std::find(ids.begin(), ids.end(), 4) == ids.end());
}

TEST_CASE("Phase 3: not_example alpha -beta", "[phase3][ranking][3.0]")
{
    SearchService ss;
    ss.add_document(1, "alpha bravo");
    ss.add_document(2, "alpha charlie");
    ss.add_document(3, "bravo charlie");
    auto scored = ss.search_scored("alpha -bravo");
    REQUIRE(doc_ids(scored) == std::vector<int>{2});
}

TEST_CASE("Phase 3: or_with_not_example alpha OR beta -gamma", "[phase3][ranking][3.0]")
{
    SearchService ss;
    ss.add_document(1, "alpha");
    ss.add_document(2, "beta");
    ss.add_document(3, "gamma");
    ss.add_document(4, "alpha gamma");
    auto scored = ss.search_scored("alpha OR beta -gamma");
    auto ids = doc_ids(scored);
    std::sort(ids.begin(), ids.end());
    REQUIRE(ids == std::vector<int>({1, 2}));
    REQUIRE(std::find(ids.begin(), ids.end(), 3) == ids.end());
    REQUIRE(std::find(ids.begin(), ids.end(), 4) == ids.end());
}

TEST_CASE("Phase 3: not_only_empty -foo", "[phase3][ranking][3.0]")
{
    SearchService ss;
    ss.add_document(1, "foo bar");
    REQUIRE(ss.search_scored("-foo").empty());
}

TEST_CASE("Phase 3: or_alone_empty", "[phase3][ranking][3.0]")
{
    SearchService ss;
    ss.add_document(1, "alpha");
    REQUIRE(ss.search_scored("OR").empty());
}

TEST_CASE("Phase 3: empty_query returns empty results", "[phase3][ranking][3.0]")
{
    SearchService ss;
    ss.add_document(1, "alpha");
    REQUIRE(ss.search_scored("").empty());
    REQUIRE(ss.search_scored("   ").empty());
}

TEST_CASE("Phase 3: empty index returns empty results", "[phase3][ranking][3.0]")
{
    SearchService ss;
    REQUIRE(ss.search_scored("alpha").empty());
}

TEST_CASE("Phase 3: tokenization_parity between index and query", "[phase3][ranking][3.0]")
{
    SearchService ss;
    ss.add_document(1, "Foo-Bar Baz");
    auto scored = ss.search_scored("foo bar baz");
    REQUIRE_FALSE(scored.empty());
    REQUIRE(scored.front().first == 1);
}

TEST_CASE("Phase 3: unmatched query returns empty results", "[phase3][ranking][3.0]")
{
    SearchService ss;
    ss.add_document(1, "alpha");
    REQUIRE(ss.search_scored("zzzznotpresent").empty());
}
