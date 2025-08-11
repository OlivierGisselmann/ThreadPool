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
    struct TaskDispatchArgs
    {
        std::uint32_t taskIndex;
        std::uint32_t groupIndex;
    };

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

        // Dispatch work to multiple groups
        inline void Dispatch(std::uint32_t taskCount, std::uint32_t groupSize, const std::function<void(TaskDispatchArgs)>& task)
        {
            if(taskCount == 0 || groupSize == 0)
                return;

            // Ceil amount of job groups
            const std::uint32_t groupCount = (taskCount + groupSize - 1) / groupSize;

            mQueuedTasks += groupCount;

            for(std::uint32_t groupIndex = 0; groupIndex < groupCount; ++groupIndex)
            {
                // Generate one task per group
                const auto& jobGroup = [taskCount, groupSize, task, groupIndex]()
                {
                    const std::uint32_t groupJobOffset = groupIndex * groupSize;
                    const std::uint32_t groupJobEnd = std::min(groupJobOffset + groupSize, taskCount);

                    TaskDispatchArgs args;
                    args.groupIndex = groupIndex;

                    for(std::uint32_t i = groupJobOffset; i < groupJobEnd; ++i)
                    {
                        args.taskIndex = i;
                        task(args);
                    }
                };

                // Push task to queue
                while(!mTaskQueue.Push(jobGroup))
                {
                    Poll();
                }

                // Wake up one thread
                mWakeUp.notify_all();
            }
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