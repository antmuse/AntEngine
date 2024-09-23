/***************************************************************************************************
 * MIT License
 *
 * Copyright (c) 2021 antmuse@live.cn/antmuse@qq.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ***************************************************************************************************/


#ifndef APP_READWRITELOCK_H
#define APP_READWRITELOCK_H


#include <atomic>
#include <thread>
#include "Spinlock.h"


namespace app {

class ReadWriteLock : public Nocopy {
public:
    static const s32 RW_UNLOCKED = 0;       //lock free
    static const s32 WRITE_LOCKED = -1;
    static const s32 MAX_RETRY_TIMES = 6;
    ReadWriteLock() {
    }
    explicit ReadWriteLock(bool write_first) : mWriteFirst(write_first) {
    }

    void lockRead();
    void lockWrite();

    void unlockRead();
    void unlockWrite();

    bool getWriteFirst() const {
        return mWriteFirst;
    }

    void setWriteFirst(bool it) {
        mWriteFirst = it;
    }

private:
    std::atomic<u32> mWriterCount = {0};
    std::atomic<s32> mReaderCount = {0};   // -1: write locked, 0: unlocked, >0: readers count
    bool mWriteFirst = true;
};

DINLINE void ReadWriteLock::lockRead() {
    s32 retry_times = 0;
    s32 read_cnt = mReaderCount.load();
    if (mWriteFirst) {
        do {
            while (read_cnt < RW_UNLOCKED || mWriterCount.load() > 0) {
                if (++retry_times == MAX_RETRY_TIMES) {
                    std::this_thread::yield();
                    retry_times = 0;
                }
                read_cnt = mReaderCount.load();
            }
        } while (!mReaderCount.compare_exchange_weak(
            read_cnt, read_cnt + 1, std::memory_order_acq_rel, std::memory_order_relaxed));
    } else {
        do {
            while (read_cnt < RW_UNLOCKED) {
                if (++retry_times == MAX_RETRY_TIMES) {
                    std::this_thread::yield();
                    retry_times = 0;
                }
                read_cnt = mReaderCount.load();
            }
        } while (!mReaderCount.compare_exchange_weak(
            read_cnt, read_cnt + 1, std::memory_order_acq_rel, std::memory_order_relaxed));
    }
}

DINLINE void ReadWriteLock::lockWrite() {
    s32 lock_free = RW_UNLOCKED;
    s32 retry_times = 0;
    mWriterCount.fetch_add(1);
    while (!mReaderCount.compare_exchange_weak(
        lock_free, WRITE_LOCKED, std::memory_order_acq_rel, std::memory_order_relaxed)) {
        lock_free = RW_UNLOCKED; // lock_free will change after CAS fail, so init agin
        if (++retry_times == MAX_RETRY_TIMES) {
            std::this_thread::yield();
            retry_times = 0;
        }
    }
    mWriterCount.fetch_sub(1);
}

DINLINE void ReadWriteLock::unlockRead() {
    mReaderCount.fetch_sub(1);
}

DINLINE void ReadWriteLock::unlockWrite() {
    mReaderCount.fetch_add(1);
}

} // namespace app



#endif // APP_READWRITELOCK_H