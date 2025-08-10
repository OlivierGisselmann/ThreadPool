#pragma once

#include <mutex>

namespace ThreadPool
{
    template<typename T, std::size_t capacity>
    class RingBuffer
    {
    public:
        inline bool Push(const T& item) noexcept
        {
            std::lock_guard<std::mutex> lock(mLock);

            // Buffer is full
            if((mWrite + 1) % capacity == mRead)
                return false;

            // Push item into array
            mData[mWrite] = item;
            mWrite = (mWrite + 1) % capacity;

            ++mCounter;

            return true;
        }

        inline bool Pop(T& item) noexcept
        {
            std::lock_guard<std::mutex> lock(mLock);

            // Buffer is empty
            if(mRead == mWrite)
                return false;

            // Pop item from array
            item = mData[mRead];
            mRead = (mRead + 1) % capacity;

            --mCounter;
            return true;
        }

        inline bool Empty() const noexcept
        {
            return mCounter == 0;
        }

    private:
        T mData[capacity];
        std::size_t mRead = 0;
        std::size_t mWrite = 0;
        std::mutex mLock;
        int mCounter = 0;
    };
}