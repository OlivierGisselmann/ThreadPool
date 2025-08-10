#include <catch2/catch_test_macros.hpp>

#include <thread_pool/thread_pool.hpp>

TEST_CASE( "ThreadPool can submit work to threads", "[thread_pool]")
{
    ThreadPool::ThreadPool pool(4);

    int counter = 0;

    pool.Submit([&]{ ++counter; });
    pool.Submit([&]{ ++counter; });
    pool.Submit([&]{ ++counter; });
    pool.Submit([&]{ ++counter; });
    
    pool.Wait();

    REQUIRE(counter == 4);
}