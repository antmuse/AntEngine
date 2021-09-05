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


#include "RingBuffer.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "Logger.h"

namespace app {

RingBuffer::RingBuffer()
    :mEmptyList(nullptr)
    , mSize(0)
    , mRet(-1) {
    //init();
}

RingBuffer::~RingBuffer() {
    uninit();
}

//mTailPos指到新的可用node
void RingBuffer::pushBack() {
    if (mEmptyList) {
        SRingBufNode* empty_block = mEmptyList;
        mTailPos.mNode->mNext = empty_block;
        mEmptyList = empty_block->mNext;
        empty_block->mNext = nullptr;
    } else {
        mTailPos.mNode->mNext = new SRingBufNode();
    }
    mTailPos.mNode = mTailPos.mNode->mNext;
    mTailPos.mPosition = 0;
}

//弹出node挂到EmptyList
void RingBuffer::popFront() {
    SRingBufNode* empty_block = mEmptyList;
    SRingBufNode* head_block = mHeadPos.mNode;
    mEmptyList = head_block;
    mHeadPos.mPosition = 0;
    mHeadPos.mNode = head_block->mNext;
    head_block->mNext = empty_block;
}

void RingBuffer::init() {
    if (!mHeadPos.mNode) {
        SRingBufNode* block = new SRingBufNode();
        mEmptyList = nullptr;
        mHeadPos.init(0, block);
        mTailPos.init(0, block);
    }
    mRet = -1;
}

void RingBuffer::uninit() {
    SRingBufNode* curr = mEmptyList;
    while (curr) {
        mEmptyList = mEmptyList->mNext;
        delete curr;
        curr = mEmptyList;
    }

    curr = mHeadPos.mNode;
    while (curr) {
        mHeadPos.mNode = mHeadPos.mNode->mNext;
        delete curr;
        curr = mHeadPos.mNode;
    }
    //mTailPos.mNode = nullptr;
    //mHeadPos.mPosition = 0;
    //mTailPos.mPosition = 0;
}

s32 RingBuffer::getSize() const {
    return mSize;
}

void RingBuffer::reset() {
    while (mHeadPos.mNode != mTailPos.mNode) {
        popFront();
    }
    mHeadPos.mPosition = 0;
    mTailPos.mPosition = 0;
    mSize = 0;
    mRet = -1;
}

void RingBuffer::write(const void* data, s32 size) {
    const s8* pos = (const s8*)data;
    s32 leftover = size;
    assert(mTailPos.mNode && "Tail block should never be nullptr");

    while (leftover > 0) {
        s32 copysz = D_RBUF_BLOCK_SIZE - mTailPos.mPosition;
        assert(mTailPos.mPosition <= D_RBUF_BLOCK_SIZE && "Tail index should always <= block size");

        if (copysz == 0) {
            pushBack();
            copysz = D_RBUF_BLOCK_SIZE;
        }

        if (copysz > leftover) {
            copysz = leftover;
        }

        memcpy(mTailPos.mNode->mData + mTailPos.mPosition, pos, copysz);

        mTailPos.mPosition += copysz;
        mSize += copysz;
        pos += copysz;
        leftover -= copysz;
    }
}

s32 RingBuffer::peekTailNode(s8** data, s32 size) {
    s32 available = D_RBUF_BLOCK_SIZE - mTailPos.mPosition;
    assert(mTailPos.mNode && "Tail block should never be nullptr");
    assert(mTailPos.mPosition <= D_RBUF_BLOCK_SIZE && "Tail index should always be <= block size");

    if (available == 0) {
        pushBack();
        available = D_RBUF_BLOCK_SIZE;
    }

    *data = mTailPos.mNode->mData + mTailPos.mPosition;
    return size > available ? available : size;
}

void RingBuffer::commitTailPos(s32 size) {
    s32 available = D_RBUF_BLOCK_SIZE - mTailPos.mPosition;
    s32 to_commit = size;
    assert(mTailPos.mNode && "Tail mNode should never be nullptr");
    if (to_commit > available) {
        to_commit = available;
    }
    mTailPos.mPosition += to_commit;
    mSize += to_commit;
    assert(mTailPos.mPosition <= D_RBUF_BLOCK_SIZE && "Tail index should always be <= block size");
}


s32 RingBuffer::read(void* data, s32 len) {
    s32 initial_size = mSize;
    s8* pos = (s8*)data;
    s32 leftover = len;
    assert(mHeadPos.mNode && "Head block should never be nullptr");

    while (leftover > 0) {
        const s8* block_pos = mHeadPos.mNode->mData + mHeadPos.mPosition;

        s32 copysz;
        if (mHeadPos.mNode == mTailPos.mNode) {
            assert(mTailPos.mPosition >= mHeadPos.mPosition && "Tail index should always be >= head index");
            copysz = mTailPos.mPosition - mHeadPos.mPosition;
            if (copysz == 0) {
                assert(initial_size >= mSize && "The ring buffer size should remain the same or decrease");
                return initial_size - mSize;
            }
        } else {
            copysz = D_RBUF_BLOCK_SIZE - mHeadPos.mPosition;
            if (copysz == 0) {
                popFront();
                continue;
            }
        }

        if (copysz > leftover) {
            copysz = leftover;
        }

        memcpy(pos, block_pos, copysz);

        mHeadPos.mPosition += copysz;
        mSize -= copysz;
        pos += copysz;
        leftover -= copysz;
    }

    assert(initial_size >= mSize && "The ring buffer size should remain the same or decrease");
    return initial_size - mSize;
}


SRingBufPos RingBuffer::peekHeadNode(SRingBufPos pos, StringView* bufs, s32* bufs_count) {
    SRingBufPos current = pos;
    s32 count = 0;
    assert(pos.mNode && "Position block should never be nullptr");

    while (count < *bufs_count) {
        StringView* buf = bufs + count;
        if (current.mNode == mTailPos.mNode) {
            s32 len = mTailPos.mPosition - current.mPosition;
            assert(mTailPos.mPosition >= current.mPosition &&
                "Tail index should always be greater than or equal to mHeadPos index");
            if (len != 0) {
                buf->mLen = (usz)len;
                buf->mData = current.mNode->mData + current.mPosition;
                count++;
            }
            *bufs_count = count;
            return mTailPos;
        } else {
            s32 len = D_RBUF_BLOCK_SIZE - current.mPosition;
            if (len != 0) {
                buf->mLen = len;
                buf->mData = current.mNode->mData + current.mPosition;
                count++;
            }
        }
        current.init(0, current.mNode->mNext);
    }
    *bufs_count = count;
    return current;
}


void RingBuffer::commitHeadPos(SRingBufPos& pos) {
    while (mHeadPos.mNode != pos.mNode && mHeadPos.mNode != mTailPos.mNode) {
        mSize -= D_RBUF_BLOCK_SIZE - mHeadPos.mPosition;
        popFront();
    }
    //if (pos.mNode == mTailPos.mNode) {
    //    mSize -= pos.mPosition - mHeadPos.mPosition;
    //}
    mSize -= pos.mPosition - mHeadPos.mPosition;
    mHeadPos = pos;
}

}//namespace app
