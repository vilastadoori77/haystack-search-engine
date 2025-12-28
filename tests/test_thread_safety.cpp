#include <catch2/catch_test_macros.hpp>
#include "core/search_service.h"
#include <cctype>
#include <string>
#include <thread>
#include <vector>
#include <atomic>

TEST_CASE("Thread safety:Concurrent searches do not crash or corrupt data")
{
    SearchService s;
    for (int idx = 1; idx <= 200; ++idx)
    {
        s.add_document(idx, "Teamcenter migration schema validation");
    }
    std::atomic<bool> ok{true};
    const int threads = 16;
    const int iters = 200;

    std::vector<std::thread> ts;
    ts.reserve(threads);

    for (int tdx = 0; tdx < threads; ++tdx)
    {
        ts.emplace_back([&]
                        {
            for(int i=0; i<iters; ++i){
                try{
                    auto results = s.search("migration schema");
                    if(results.empty()) ok = false;
                } catch(...){
                    ok = false;     
                }
            } });
    }

    for (auto &th : ts)
        th.join();
    REQUIRE(ok.load() == true);
}

TEST_CASE("Thread safety:add_document + search concurrently does not crash or corrupt data")
{
    SearchService s;
    for (int i = 1; i <= 50; ++i)
    {
        s.add_document(i, "hello world migration schema validation");
    }
    std::atomic<bool> ok{true};

    std::thread writer([&]
                       {
                           try
                           {
                               for (int i = 51; i <= 200; ++i)
                               {
                                   s.add_document(i, "Teamcenter migration schema validation");
                               }
                           }
                           catch (...)
                           {
                               ok = false;
                           } });

    std::thread reader([&]
                       {
        try {
            for (int i = 0; i < 200; ++i)
            {
                auto results = s.search("migration schema");
                (void)results;
            }
        }
        catch (...)
        {
            ok = false;
        } });

    writer.join();
    reader.join();
    REQUIRE(ok.load() == true);
}