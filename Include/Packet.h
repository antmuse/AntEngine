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


#ifndef APP_PACKET_H
#define APP_PACKET_H

#include "Config.h"

namespace app {

class Packet {
public:
    Packet();

    Packet(usz iSize);

    ~Packet();

    Packet(const Packet& other) {
        *this = other;
    }

    Packet(Packet&& it)noexcept :mUsed(it.mUsed), mAllocated(it.mAllocated), mData(it.mData) {
        it.mData = nullptr;
        it.mAllocated = 0;
        it.mUsed = 0;
    }

    Packet& operator=(const Packet& other);

    Packet& operator=(Packet&& it)noexcept {
        swap(it);
        return *this;
    }

    void swap(Packet& it)noexcept {
        s8* dat = mData;
        usz used = mUsed;
        usz allc = mAllocated;

        mUsed = it.mUsed;
        mAllocated = it.mAllocated;
        mData = it.mData;

        it.mData = dat;
        it.mAllocated = allc;
        it.mUsed = used;
    }

    void reallocate(usz size);

    DFINLINE s8& operator[](usz index) {
        return mData[index];
    }

    DFINLINE const s8& operator[](usz index) const {
        return mData[index];
    }

    /**
    *@brief 收缩容量使其不超过maximum.
    *@param maximum The max cache size.
    */
    void shrink(usz maximum);

    void resize(usz size);

    DFINLINE usz size() const {
        return mUsed;
    }

    DFINLINE usz capacity() const {
        return mAllocated;
    }


    DFINLINE s8* getWritePointer()const {
        return mData + mUsed;
    }

    DFINLINE usz getWriteSize() const {
        return mAllocated - mUsed;
    }

    DFINLINE s8* getEnd() const {
        return mData + mUsed;
    }


    DFINLINE s8* getPointer()const {
        return mData;
    }


    usz write(const void* iData, usz iSize);

    usz writeU8(u8 ch);

    usz writeU16(u16 ch);

    usz writeU32(u32 ch);

    /**
    *@brief 从头部清理指定字节数.剩余字节数前移
    *@param iSize 删除字节数.
    *@return 剩余字节数.
    */
    usz clear(usz iSize = (-1));

protected:
    usz mUsed;
    usz mAllocated;
    s8* mData;
};

}// end namespace

#endif //APP_PACKET_H