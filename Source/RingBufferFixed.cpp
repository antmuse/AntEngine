#include "RingBufferFixed.h"
#include <string.h>
#include "Logger.h"

namespace app {
static usz AppUpToPower2(usz it) {
    u32 ret = 2;
    for (; ret < it; ret <<= 1) {
    }
    return ret;
}

class BlockHead {
public:
    u32 mBlockSize = 0;
    u32 mMsgSize = 0;
};


RingBufferFixed::RingBufferFixed() {
}

RingBufferFixed::~RingBufferFixed() {
}

void RingBufferFixed::stop() {
    mRunning = 0;
    mFutex.tryUnlock();
}


bool RingBufferFixed::init(void* buf, usz len) {
    if (this != buf || len < sizeof(*this) * 2) {
        DLOG(ELL_ERROR, "init: rpos=%llu, wpos=%llu, count=%u, capacity= %llu, ref = %d", mPosRead, mPosWrite,
            mMsgCount, mCapacity, mRefCount);
        return false;
    }
    len -= sizeof(*this);
    usz buflen = AppUpToPower2(len);
    while (buflen > len) {
        buflen >>= 1;
    }
    mRefCount = 1;
    mMsgCount = 0;
    mRunning = 0;
    mFlag = 0;
    mPosRead = 0;
    mPosWrite = 0;
    mCapacity = 0;     // must be pow of 2
    mCapacityMask = 0; // = (mCapacity-1)
    new (&mLock) Spinlock();
    new (&mFutex) Futex();
    mRunning = 1;
    mCapacity = buflen;
    mCapacityMask = mCapacity - 1;
    DLOG(ELL_INFO, "init: rpos=%llu, wpos=%llu, count=%u, capacity= %llu, ref = %d", mPosRead, mPosWrite, mMsgCount,
        mCapacity, mRefCount);
    return buflen > 0;
}


ssz RingBufferFixed::read(void* out, usz len) {
    if (mPosRead != mPosWrite) {
        const usz offset = mPosRead & mCapacityMask;
        const BlockHead* head = reinterpret_cast<BlockHead*>(mBuffer + offset);
        const usz msg_len = head->mMsgSize;
        if (msg_len <= len) {
            usz block_len = head->mBlockSize;
            usz partlen = AppMin(msg_len, mCapacity - offset - sizeof(BlockHead));
            memcpy(out, head + 1, partlen);
            memcpy(static_cast<s8*>(out) + partlen, mBuffer, msg_len - partlen);
            mPosRead += block_len;
            --mMsgCount;
            return static_cast<ssz>(msg_len); // success read
        } else {
            return EE_INVALID_PARAM; // fail, need more cache
        }
    }
    return 0; // empty
}


ssz RingBufferFixed::write(const void* msg, usz len) {
    const usz leftover = mCapacity - (mPosWrite - mPosRead); // used = mPosWrite-mPosRead
    const usz block_len = len + sizeof(BlockHead);
    if (block_len <= leftover) {
        usz offset = mPosWrite & mCapacityMask;
        BlockHead* head = reinterpret_cast<BlockHead*>(mBuffer + offset);
        head->mBlockSize = block_len;
        head->mMsgSize = len;
        usz part_len = AppMin(len, mCapacity - offset - sizeof(BlockHead));
        memcpy(head + 1, msg, part_len);
        memcpy(mBuffer, static_cast<const s8*>(msg) + part_len, len - part_len);
        mPosWrite += block_len;
        ++mMsgCount;

        // write padding if...
        offset = mPosWrite & mCapacityMask;
        part_len = mCapacity - offset;
        if (part_len < sizeof(BlockHead)) {
            head->mBlockSize += part_len;
            mPosWrite += part_len;
            DLOG(ELL_INFO, "write padding: %llu/%llu, count=%llu, pad len= %llu, msglen = %llu", mPosRead, mPosWrite,
                mMsgCount, part_len, len);
        }
        return static_cast<ssz>(len); // success write
    }

    // return 0 if full, else fail code
    return block_len > mCapacity ? EE_INVALID_PARAM : 0;
}



} // namespace app
