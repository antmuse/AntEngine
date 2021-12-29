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


#include "Packet.h"
#include <string.h>
#include <string>

namespace app {


Packet::Packet() :
    mUsed(0),
    mAllocated(1024) {
    mData = (s8*)malloc(mAllocated);
}


Packet::Packet(usz iSize) :
    mUsed(0),
    mAllocated(iSize) {
    if (mAllocated > 0 && mAllocated < 8) {
        mAllocated = 8;
    }
    mData = mAllocated > 0 ? (s8*)malloc(mAllocated) : nullptr;
}


Packet::~Packet() {
    if (mData) {
        free(mData);
        mData = nullptr;
        mUsed = 0;
        mAllocated = 0;
    }
}


void Packet::shrink(usz maximum) {
    if (mAllocated > maximum) {
        ::free(mData);
        mData = (s8*) ::malloc(maximum);
        DASSERT(mData);
        mAllocated = maximum;
    }
    mUsed = 0;
}


void Packet::reallocate(usz size) {
    DASSERT(size <= 1024 * 1024 * 1024);//assume <= 1G
    if (mAllocated < size) {
        size += (mAllocated < 512 ?
            (mAllocated < 8 ? 8 : mAllocated)
            : mAllocated >> 2);

        s8* buffer = (s8*) ::malloc(size);
        DASSERT(buffer);

        if (mUsed > 0) {
            ::memcpy(buffer, mData, mUsed);
        }
        ::free(mData);
        mData = buffer;
        mAllocated = size;
    }
}


void Packet::resize(usz size) {
    if (mAllocated < size) {
        reallocate(size);
    }
    mUsed = size;
}


Packet& Packet::operator= (const Packet& other) {
    if (this == &other) {
        return *this;
    }
    resize(other.size());
    ::memcpy(mData, other.getPointer(), other.size());
    return *this;
}



usz Packet::clear(usz position) {
    if (position > 0 && position < mUsed) {
        mUsed = mUsed - position;
        ::memmove(mData, mData + position, mUsed);
    } else {
        mUsed = (0 == position) ? mUsed : 0;
    }
    return mUsed;
}


usz Packet::write(const void* iData, usz iLength) {
    DASSERT(iData && iLength >= 0);

    usz osize = mUsed;
    resize(osize + iLength);
    ::memcpy(mData + osize, iData, iLength);
    return mUsed;
}

usz Packet::writeU8(u8 ch) {
    usz osize = mUsed;
    resize(osize + sizeof(ch));
    mData[osize] = ch;
    return mUsed;
}

usz Packet::writeU16(u16 ch) {
    usz osize = mUsed;
    resize(osize + sizeof(ch));
    *(u16*)(mData + osize) = ch;
    return mUsed;
}

usz Packet::writeU32(u32 ch) {
    usz osize = mUsed;
    resize(osize + sizeof(ch));
    *(u32*)(mData + osize) = ch;
    return mUsed;
}

}// end namespace
