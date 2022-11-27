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


#ifndef APP_CSPINLOCK_H
#define APP_CSPINLOCK_H

#include <atomic>
#include "Nocopy.h"

namespace app {

/**
* @brief a spinlock base on atomic operations.
* @note Do't use spinlock on mononuclear CPU, or havy calculation tasks.
* Spinlock is not recursive lock.
*/
class Spinlock : public Nocopy {
public:
    Spinlock() :mValue(0) {
    }

    ~Spinlock() {
    }

    /**
    * @note It's a busing CPU wait when spin.
    */
    void lock() {
        s32 val = 0;
        while (!mValue.compare_exchange_strong(val, 1)) {
            //busy waiting
            val = 0;
        }
    }

    bool tryLock() {
        s32 val = 0;
        return mValue.compare_exchange_strong(val, 1);
    }

    bool tryUnlock() {
        s32 val = 1;
        return mValue.compare_exchange_strong(val, 0);
    }

    void unlock() {
        mValue.store(0);
    }

private:
    //1=locked, 0=unlocked
    std::atomic<s32> mValue;
};


template<class T>
class CAutoLock {
public:
    CAutoLock(T& it) : mLock(it) {
        mLock.lock();
    }

    ~CAutoLock() {
        mLock.unlock();
    }

private:
    T& mLock;
    CAutoLock() = delete;
    CAutoLock(CAutoLock&) = delete;
    CAutoLock(CAutoLock&&) = delete;
    CAutoLock& operator=(CAutoLock&) = delete;
    CAutoLock& operator=(CAutoLock&&) = delete;
};


} //namespace app

#endif //APP_CSPINLOCK_H
