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


#ifndef APP_RINGBUFFER_H
#define APP_RINGBUFFER_H

#include "Nocopy.h"
#include "Strings.h"

#define D_RBUF_BLOCK_SIZE (4 * 1024)

namespace app {

struct SRingBufNode {
    s8 mData[D_RBUF_BLOCK_SIZE];
    SRingBufNode* mNext;

    SRingBufNode() : mNext(nullptr) {
    }

    ~SRingBufNode() {
    }
};

struct SRingBufPos {
    s32 mPosition;
    SRingBufNode* mNode;
    SRingBufPos() :mPosition(0), mNode(nullptr) {
    }
    SRingBufPos(s32 pos, SRingBufNode* bk)
        :mPosition(pos), mNode(bk) {
    }
    void init(s32 pos, SRingBufNode* bk) {
        mPosition = pos;
        mNode = bk;
    }
};

class RingBuffer : public Nocopy {
public:
    RingBuffer();

    ~RingBuffer();

    void swap(RingBuffer& it) {
        SRingBufPos tail = mTailPos;
        SRingBufPos hd = mHeadPos;
        s32 sz = mSize;
        long rt = mRet;

        mTailPos = it.mTailPos;
        mHeadPos = it.mHeadPos;
        mSize = it.mSize;
        mRet = it.mRet;

        it.mTailPos = tail;
        it.mHeadPos = hd;
        it.mSize = sz;
        it.mRet = rt;
    }


    void init();

    void uninit();

    void reset();

    long getRet()const {
        return mRet;
    }

    void setRet(long val) {
        mRet = val;
    }

    const SRingBufPos& getHead()const {
        return mHeadPos;
    }

    const SRingBufPos& getTail()const {
        return mTailPos;
    }

    s32 read(void* data, s32 len);

    void write(const void* data, s32 size);

    bool rewrite(SRingBufPos& pos, const void* data, s32 size);

    s32 getSize() const;

    /**
    * @brief peek data bufs when writing.
    */
    s32 peekTailNode(s8** data, s32 size);
    s32 peekTailNode(s32 reserved, s8** data, s32 size);

    /**
    * @param how many bytes writed in peeked tail cache before.
    */
    void commitTailPos(s32 writedBytes);

    /**
    * @brief peek data bufs when reading.
    */
    SRingBufPos peekHeadNode(SRingBufPos pos, StringView* bufs, s32* bufs_count);

    /**
    * @param pos readed position of peeked head cache before.
    */
    void commitHeadPos(const SRingBufPos& pos);


    StringView peekHead();
    void commitHead(s32 used);

private:
    SRingBufPos mTailPos;
    SRingBufPos mHeadPos;
    s32 mSize;
    long mRet; //for tls


    void popFront();

    void pushBack();
};

}//namespace app

#endif //APP_RINGBUFFER_H