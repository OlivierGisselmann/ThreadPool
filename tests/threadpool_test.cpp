#include <catch2/catch_test_macros.hpp>

#include <thread_pool/ring_buffer.hpp>

TEST_CASE( "Ring buffer pushes and pops", "[thread_pool]" ) {
    ThreadPool::RingBuffer<int, 50> queue;

    for(int i = 0; i < 49; ++i)
    {
        queue.Push(i);
    }

    for(int i = 0; i < 49; ++i)
    {
        int result = 0;
        queue.Pop(result);
        REQUIRE( result == i );
    }
}