#include <catch2/catch_test_macros.hpp>

#include <thread_pool/ring_buffer.hpp>

#include <thread>

TEST_CASE( "Empty() ring buffer returns value", "[ring_buffer]")
{
    ThreadPool::RingBuffer<int, 5> queue;

    REQUIRE(queue.Empty() == true);

    queue.Push(1);

    REQUIRE(queue.Empty() == false);
}

TEST_CASE( "Pop empty ring buffer returns false", "[ring_buffer]")
{
    ThreadPool::RingBuffer<int, 5> queue;
    int result = 0;

    REQUIRE(queue.Pop(result) == false);
}

TEST_CASE( "Push full ring buffer returns false", "[ring_buffer]")
{
    ThreadPool::RingBuffer<int, 1> queue;

    queue.Push(42);

    REQUIRE(queue.Push(21) == false);
}

TEST_CASE( "Ring buffer pushes and pops", "[ring_buffer]" )
{
    // Create a ring queue with 50 int slots
    ThreadPool::RingBuffer<int, 50> queue;
    int result = 0;

    // Can fill the queue
    for(int i = 0; i < 49; ++i)
    {
        REQUIRE(queue.Push(i) == true);
    }

    // Can pop queue data
    for(int i = 0; i < 49; ++i)
    {
        queue.Pop(result);
        REQUIRE( result == i );
    }
}

TEST_CASE( "Ring buffer has correct values", "[ring_buffer]")
{
    ThreadPool::RingBuffer<int, 4> queue;
    int result = 0;

    // FIFO
    queue.Push(21);
    queue.Push(42);
    queue.Push(96);

    queue.Pop(result);
    REQUIRE(result == 21);

    queue.Pop(result);
    REQUIRE(result == 42);

    queue.Pop(result);
    REQUIRE(result == 96);
}

TEST_CASE( "Ring buffer is thread safe", "[ring_buffer]")
{
    ThreadPool::RingBuffer<int, 5> queue;
    int result = 0;

    std::thread first([&queue]
    {
        queue.Push(1);
    });

    std::thread second([&queue, &result]
    {
        queue.Pop(result);
        queue.Push(2);
    });

    first.join();
    second.join();
    
    queue.Pop(result);
    REQUIRE(result == 2);
}