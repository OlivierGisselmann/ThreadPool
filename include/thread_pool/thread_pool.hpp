#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <thread>
#include <vector>

#include "ring_buffer.hpp"

namespace ThreadPool
{
    class ThreadPool
    {
    public:
        explicit ThreadPool(std::size_t desiredThreads = std::thread::hardware_concurrency())
        : mShutdown(false), mQueuedTasks(0)
        {
            mCompletedTasks.store(0);

            auto numThreads = std::max((std::size_t)1, desiredThreads);

            // Allocate space for threads
            mThreads.reserve(numThreads);

            // Create x threads with fetch task loop running inside
            for(std::size_t i = 0; i < numThreads; ++i)
            {
                mThreads.emplace_back([this]{
                    std::function<void()> task;

                    for(;;)
                    {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(mLock);

                            mWakeUp.wait(lock, [this]()
                            {
                                return mShutdown || !mTaskQueue.Empty();
                            });

                            if(mShutdown && mTaskQueue.Empty())
                                break;

                            if(mTaskQueue.Empty())
                                continue;

                            mTaskQueue.Pop(task);
                        }

                        task();
                        mCompletedTasks.fetch_add(1);
                    }
                });
            }
        }

        // Deleted copy constructor, copy assignment operator and move constructor
        ThreadPool(ThreadPool&) = delete;
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        // Ensure all threads are joined when destroying
        ~ThreadPool()
        {
            for(auto& thread : mThreads)
            {
                if(thread.joinable())
                    thread.join();
            }
        }

        // Using auto to define return type at compile time
        template<typename F, typename... Args>
        inline auto Submit(F&& f, Args&&... args) ->std::future<decltype(f(args...))>
        {
            auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            auto funcPtr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

            std::future<std::result_of_t<F(Args...)>> futureObject = funcPtr->get_future();

            ++mQueuedTasks;

            // Push new task to queue
            while(!mTaskQueue.Push(
                [funcPtr]()
                {
                    (*funcPtr)();
                }
            ))
            {
                Poll();
            }

            mWakeUp.notify_one();

            return futureObject;
        }

        inline void Wait() noexcept
        {
            while(IsBusy())
            {
                Poll();
            }

            // Main thread notifies all workers to stop
            mShutdown = true;
            mWakeUp.notify_all();
        }

    private:
        // Put current thread to sleep and wake another one
        inline void Poll() noexcept
        {
            mWakeUp.notify_one();
            std::this_thread::yield();
        }

        inline bool IsBusy() const noexcept
        {
            return mCompletedTasks.load() < mQueuedTasks;
        }

        std::vector<std::thread> mThreads;
        RingBuffer<std::function<void()>, 256> mTaskQueue;

        std::condition_variable mWakeUp;
        mutable std::mutex mLock;
        bool mShutdown = false;

        std::uint64_t mQueuedTasks = 0;
        std::atomic<std::uint64_t> mCompletedTasks;
    };
}