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


#ifndef APP_INTID_H
#define	APP_INTID_H

#include "Config.h"


namespace app {


class ID128 {
public:
    ID128() :mLow(0), mHigh(0) { }

    ID128(s64 low, s64 high) :mLow(low), mHigh(high) { }

    ~ID128() { }

    ID128(const ID128& it) :mLow(it.mLow), mHigh(it.mHigh) {
    }

    ID128(const ID128&& it)noexcept :mLow(it.mLow), mHigh(it.mHigh) {
    }

    ID128(const void* it) {
        mLow = *(s64*)it;
        mHigh = *(((s64*)it) + 1);
    }

    void clear() {
        mLow = 0;
        mHigh = 0;
    }


    ID128& operator=(const ID128&& it)noexcept {
        mLow = it.mLow;
        mHigh = it.mHigh;
        return *this;
    }

    ID128& operator=(const ID128& it) {
        mLow = it.mLow;
        mHigh = it.mHigh;
        return *this;
    }

    ID128& operator=(const void* it) {
        mLow = *(s64*)it;
        mHigh = *(((s64*)it) + 1);
        return *this;
    }

    bool operator>(const ID128& it)const {
        if(mHigh > it.mHigh) {
            return true;
        } else if(mHigh < it.mHigh) {
            return false;
        }
        return (mLow > it.mLow);
    }

    bool operator<(const ID128& it)const {
        if(mHigh < it.mHigh) {
            return true;
        } else if(mHigh > it.mHigh) {
            return false;
        }
        return (mLow < it.mLow);
    }

    bool operator==(const ID128& it)const {
        return (mLow == it.mLow && mHigh == it.mHigh);
    }

    bool operator!=(const ID128& it)const {
        return (mLow != it.mLow || mHigh != it.mHigh);
    }

    s64 mLow;
    s64 mHigh;
};

#pragma pack(2)
class ID144 {
public:
    ID144() :mLow(0), mHigh(0), mTail(0) { }

    ID144(s64 low, s64 high, s16 tail) :mLow(low), mHigh(high), mTail(tail) { }

    ~ID144() { }

    ID144(const ID144& it) :mLow(it.mLow), mHigh(it.mHigh), mTail(it.mTail) {
    }

    ID144(const ID144&& it)noexcept :mLow(it.mLow), mHigh(it.mHigh), mTail(it.mTail) {
    }

    ID144(const void* it) {
        mLow = *(s64*)it;
        mHigh = *(((s64*)it) + 1);
        mTail = *(((s16*)it) + 8);
    }

    void clear() {
        mLow = 0;
        mHigh = 0;
        mTail = 0;
    }

    ID144& operator=(const ID144& it) {
        mLow = it.mLow;
        mHigh = it.mHigh;
        mTail = it.mTail;
        return *this;
    }

    ID144& operator=(const ID144&& it)noexcept {
        mLow = it.mLow;
        mHigh = it.mHigh;
        mTail = it.mTail;
        return *this;
    }

    ID144& operator=(const void* it) {
        mLow = *(s64*)it;
        mHigh = *(((s64*)it) + 1);
        mTail = *(((s16*)it) + 8);
        return *this;
    }

    bool operator>(const ID144& it)const {
        if(mTail > it.mTail) {
            return true;
        } else if(mTail < it.mTail) {
            return false;
        }
        if(mHigh > it.mHigh) {
            return true;
        } else if(mHigh < it.mHigh) {
            return false;
        }
        return (mLow > it.mLow);
    }

    bool operator<(const ID144& it)const {
        if(mTail < it.mTail) {
            return true;
        } else if(mTail > it.mTail) {
            return false;
        }
        if(mHigh < it.mHigh) {
            return true;
        } else if(mHigh > it.mHigh) {
            return false;
        }
        return (mLow < it.mLow);
    }

    bool operator==(const ID144& it)const {
        return (mLow == it.mLow && mHigh == it.mHigh && mTail == it.mTail);
    }

    bool operator!=(const ID144& it)const {
        return (mLow != it.mLow || mHigh != it.mHigh || mTail != it.mTail);
    }

    s64 mLow;
    s64 mHigh;
    s16 mTail;
};
#pragma pack()


} //namespace app

#endif //APP_INTID_H
