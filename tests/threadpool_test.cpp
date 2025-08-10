#include <catch2/catch_test_macros.hpp>

#include <thread_pool/thread_pool.hpp>

#include <iostream>

TEST_CASE( "ThreadPool can submit work to multiple threads", "[thread_pool]")
{
    ThreadPool::ThreadPool pool(4);

    for(int i = 0; i < 8; ++i)
    {
        pool.Submit([i]{
            return i + i;
        });
    }

    pool.Wait();
}

TEST_CASE( "ThreadPool returns values with std::future", "[thread_pool]")
{
    ThreadPool::ThreadPool pool;

    std::vector<std::future<int>> results;

    for(int i = 0; i < 8; ++i)
    {
        auto future = pool.Submit([i]{
            return i + i;
        });

        results.emplace_back(std::move(future));
    }

    pool.Wait();

    for(auto& result : results)
    {
        static int i = 0;
        REQUIRE(result.get() == i + i);
        ++i;
    }
}