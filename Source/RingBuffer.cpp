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
#include <stdlib.h>
#include <string.h>
#include "Logger.h"
#include "Node.h"

namespace app {

RingBuffer::RingBuffer()
    : mSize(0), mRet(-1) {
    //init();
}

RingBuffer::~RingBuffer() {
    uninit();
}


void RingBuffer::pushBack() {
    if (mTailPos.mNode->mNext != mHeadPos.mNode) {
        mTailPos.mNode = mTailPos.mNode->mNext;
    } else {
        AppPushRingQueueTail_1(mTailPos.mNode, new SRingBufNode());
    }
    mTailPos.mPosition = 0;
}


void RingBuffer::popFront() {
    mHeadPos.mPosition = 0;
    mHeadPos.mNode = mHeadPos.mNode->mNext;
}

void RingBuffer::init() {
    if (!mHeadPos.mNode) {
        AppPushRingQueueTail_1(mHeadPos.mNode, new SRingBufNode());
        mHeadPos.mPosition = 0;
        mTailPos = mHeadPos;
    }
    mRet = -1;
}

void RingBuffer::uninit() {
    SRingBufNode* curr = AppPopRingQueueHead_1(mHeadPos.mNode);
    while (curr) {
        delete curr;
        curr = AppPopRingQueueHead_1(mHeadPos.mNode);
    }
    DASSERT(nullptr == mHeadPos.mNode);
    mTailPos.mNode = nullptr;
    mHeadPos.mPosition = 0;
    mTailPos.mPosition = 0;
    mSize = 0;
    mRet = -1;
}

s32 RingBuffer::getSize() const {
    return mSize;
}

void RingBuffer::reset() {
    while (mHeadPos.mNode != mTailPos.mNode) {
        SRingBufNode* nd = AppPopRingQueueHead_1(mHeadPos.mNode);
        delete nd;
    }
    mHeadPos.mPosition = 0;
    mTailPos.mPosition = 0;
    mSize = 0;
    mRet = -1;
}


void RingBuffer::write(const void* data, s32 size) {
    const s8* pos = (const s8*)data;
    s32 leftover = size;
    DASSERT(nullptr!=mTailPos.mNode);

    while (leftover > 0) {
        s32 copysz = sizeof(SRingBufNode::mData) - mTailPos.mPosition;
        DASSERT(mTailPos.mPosition <= sizeof(SRingBufNode::mData));

        if (copysz == 0) {
            pushBack();
            copysz = sizeof(SRingBufNode::mData);
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

bool RingBuffer::rewrite(SRingBufPos& ndpos, const void* data, s32 size) {
    const s8* pos = (const s8*)data;
    s32 leftover = size;
    DASSERT(nullptr != ndpos.mNode);

    while (leftover > 0) {
        s32 copysz = sizeof(SRingBufNode::mData) - ndpos.mPosition;
        DASSERT(ndpos.mPosition <= sizeof(SRingBufNode::mData));

        if (copysz == 0) {
            //@note can't pushBack() here
            if (ndpos.mNode->mNext == mTailPos.mNode && mTailPos.mPosition < leftover) {
                return false;
            }
            ndpos.mNode = ndpos.mNode->mNext;
            ndpos.mPosition = 0;
            copysz = sizeof(SRingBufNode::mData);
        }

        if (copysz > leftover) {
            copysz = leftover;
        }

        memcpy(ndpos.mNode->mData + ndpos.mPosition, pos, copysz);

        ndpos.mPosition += copysz;
        mSize += copysz;
        pos += copysz;
        leftover -= copysz;
    }
    return true;
}


s32 RingBuffer::peekTailNode(s8** data, s32 size) {
    DASSERT(nullptr!=mTailPos.mNode);
    DASSERT(mTailPos.mPosition <= sizeof(SRingBufNode::mData));

    s32 available = sizeof(SRingBufNode::mData) - mTailPos.mPosition;
    if (available == 0) {
        pushBack();
        available = sizeof(SRingBufNode::mData);
    }

    *data = mTailPos.mNode->mData + mTailPos.mPosition;
    return size > available ? available : size;
}

s32 RingBuffer::peekTailNode(s32 reserved, s8** data, s32 size) {
    DASSERT(nullptr != mTailPos.mNode && reserved >= 0);
    DASSERT(mTailPos.mPosition <= sizeof(SRingBufNode::mData));

    s32 available = sizeof(SRingBufNode::mData) - mTailPos.mPosition;
    while (true) {
        if (reserved >= available) {
            reserved -= available;
            pushBack();
            available = sizeof(SRingBufNode::mData);
        } else {
            mTailPos.mPosition += reserved;
            available -= reserved;
            break;
        }
    }

    *data = mTailPos.mNode->mData + mTailPos.mPosition;
    return size > available ? available : size;
}

void RingBuffer::commitTailPos(s32 size) {
    s32 available = sizeof(SRingBufNode::mData) - mTailPos.mPosition;
    s32 to_commit = size;
    DASSERT(nullptr!=mTailPos.mNode);
    if (to_commit > available) {
        to_commit = available;
    }
    mTailPos.mPosition += to_commit;
    mSize += to_commit;
    DASSERT(mTailPos.mPosition <= sizeof(SRingBufNode::mData));
}


s32 RingBuffer::read(void* data, s32 len) {
    s32 initial_size = mSize;
    s8* pos = (s8*)data;
    s32 leftover = len;
    DASSERT(nullptr!=mHeadPos.mNode);

    while (leftover > 0) {
        const s8* block_pos = mHeadPos.mNode->mData + mHeadPos.mPosition;

        s32 copysz;
        if (mHeadPos.mNode == mTailPos.mNode) {
            DASSERT(mTailPos.mPosition >= mHeadPos.mPosition);
            copysz = mTailPos.mPosition - mHeadPos.mPosition;
            if (copysz == 0) {
                DASSERT(initial_size >= mSize);
                return initial_size - mSize;
            }
        } else {
            copysz = sizeof(SRingBufNode::mData) - mHeadPos.mPosition;
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

    DASSERT(initial_size >= mSize);
    return initial_size - mSize;
}

StringView RingBuffer::peekHead() {
    StringView ret;
    if (mHeadPos.mNode == mTailPos.mNode) {
        s32 len = mTailPos.mPosition - mHeadPos.mPosition;
        DASSERT(len >= 0);
        ret.mLen = AppMin(len, mSize);
        ret.mData = mHeadPos.mNode->mData + mHeadPos.mPosition;
    } else {
        s32 len = sizeof(SRingBufNode::mData) - mHeadPos.mPosition;
        DASSERT(len >= 0);
        ret.mLen = (usz)len;
        ret.mData = mHeadPos.mNode->mData + mHeadPos.mPosition;
    }
    return ret;
}

void RingBuffer::commitHead(s32 used) {
    DASSERT(used >= 0 && used <= sizeof(SRingBufNode::mData) && mSize >= used);
    mSize -= used;
    if (mHeadPos.mNode == mTailPos.mNode) {
        DASSERT(mTailPos.mPosition - mHeadPos.mPosition >= used);
        mHeadPos.mPosition += used;
    } else {
        s32 leftover = sizeof(SRingBufNode::mData) - mHeadPos.mPosition;
        if (leftover == used) {
            popFront();
        } else {
            mHeadPos.mPosition += used;
        }
    }
}

SRingBufPos RingBuffer::peekHeadNode(SRingBufPos pos, StringView* bufs, s32* bufs_count) {
    SRingBufPos current = pos;
    s32 count = 0;
    DASSERT(nullptr!=pos.mNode);

    while (count < *bufs_count) {
        StringView* buf = bufs + count;
        if (current.mNode == mTailPos.mNode) {
            s32 len = mTailPos.mPosition - current.mPosition;
            DASSERT(mTailPos.mPosition >= current.mPosition);
            if (len != 0) {
                buf->mLen = (usz)len;
                buf->mData = current.mNode->mData + current.mPosition;
                count++;
            }
            *bufs_count = count;
            return mTailPos;
        } else {
            s32 len = sizeof(SRingBufNode::mData) - current.mPosition;
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


void RingBuffer::commitHeadPos(const SRingBufPos& pos) {
    while (mHeadPos.mNode != pos.mNode && mHeadPos.mNode != mTailPos.mNode) {
        mSize -= sizeof(SRingBufNode::mData) - mHeadPos.mPosition;
        popFront();
    }
    mSize -= pos.mPosition - mHeadPos.mPosition;
    mHeadPos = pos;
}

}//namespace app
