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


#pragma once
#ifndef APP_FIXEDRINGBUFFER_H
#define APP_FIXEDRINGBUFFER_H

#include "Nocopy.h"
#include "Futex.h"
#include "Spinlock.h"

namespace app {

/**
 * @brief A ring buffer that has fixed cache size.
 * Supports reading in units of message blocks.
 */
class RingBufferFixed : public Nocopy {
public:
    RingBufferFixed();

    ~RingBufferFixed();

    bool init(void* buf, usz len);

    void stop();

    /** @return readed size in bytes. */
    ssz read(void* out, usz len);

    /** @return writed size in bytes. */
    ssz write(const void* msg, usz len);

    Spinlock& getSpinlock() {
        return mLock;
    }

    Futex& getFutex() {
        return mFutex;
    }

    s32 getRefCount() const {
        return mRefCount;
    }

    u32 getMsgCount() const {
        return mMsgCount;
    }

    usz getCapacity() const {
        return mCapacity;
    }

    s32 grab() {
        return ++mRefCount;
    }
    s32 drop() {
        return --mRefCount;
    }

private:
    u32 mMsgCount = 0;
    s32 mRefCount = 1;
    s32 mRunning = 1;
    u32 mFlag = 0;
    Spinlock mLock;
    Futex mFutex;
    usz mPosRead = 0;
    usz mPosWrite = 0;
    usz mCapacity = 0;     // must be pow of 2
    usz mCapacityMask = 0; // = (mCapacity-1)
    s8 mBuffer[0];
};


} // namespace app

#endif // APP_FIXEDRINGBUFFER_H