#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>
#include <vector>

#include "ring_buffer.hpp"

namespace ThreadPool
{
    class ThreadPool
    {
    public:
        ThreadPool(std::size_t desiredThreads = std::thread::hardware_concurrency())
        : mShutdown(false), mQueuedTasks(0)
        {
            mCompletedTasks.store(0);

            auto numThreads = std::max((std::size_t)1, desiredThreads);

            for(std::size_t i = 0; i < numThreads; ++i)
            {
                mThreads.emplace_back([this]{
                    std::function<void()> task;

                    while(!mShutdown)
                    {
                        auto result = mTaskQueue.Pop(task);

                        if(result)
                        {
                            task();
                            mCompletedTasks.fetch_add(1);
                        }
                        else
                        {
                            std::unique_lock<std::mutex> lock(mLock);
                            mWakeUp.wait(lock);
                        }
                    }
                });
            }
        }

        // Deleted copy constructor & copy assignment operator
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        ~ThreadPool()
        {
            for(auto& thread : mThreads)
            {
                if(thread.joinable())
                    thread.join();
            }
        }

        inline void Submit(const std::function<void()>& task)
        {
            ++mQueuedTasks;

            // Try to push task until it works
            while(!mTaskQueue.Push(task))
            {
                Poll();
            }

            mWakeUp.notify_one();
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