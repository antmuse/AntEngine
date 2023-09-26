#include "RingBlocks.h"
#include <atomic>
#include <string.h>

namespace app {

RingBlocks::RingBlocks() : mCache(nullptr), mMask(0), mHead(0), mTail(0), mBlockSize(4096) {
}

RingBlocks::~RingBlocks() {
    close();
}

u32 RingBlocks::upToPower2(u32 it) {
    u32 ret = 2;
    for (; ret < it; ret <<= 1) {
    }
    return ret;
}

void RingBlocks::open(u32 slots, u16 blocksize) {
    if (mCache) {
        return;
    }
    mBlockSize = AppMin(upToPower2(AppMax<u32>(blocksize, 32U)), 0x8000U);
    slots = AppMin(upToPower2(AppMax(2U, slots)), 0x8000U);
    mMask = slots - 1;
    mHead = 0;
    mTail = 0;
    mCache = new s8[static_cast<usz>(slots) * mBlockSize];
    mBlockSize -= sizeof(RingBlocks::Block);
}

void RingBlocks::close() {
    if (mCache) {
        delete[] mCache;
        mCache = nullptr;
    }
}

usz RingBlocks::write(const void* buf, usz size) {
    const usz blocksz = getBlockSize();
    usz ret = 0;
    RingBlocks::Block* block;
    while (ret < size) {
        block = getTailBlock();
        if (!block) {
            break;
        }
        block->mUsed = static_cast<u16>(AppMin(blocksz, size - ret));
        memcpy(block->mBuf, reinterpret_cast<const s8*>(buf) + ret, block->mUsed);
        block->mOffset = 0;
        ret += block->mUsed;
        commitTail();
    }
    return ret;
}

usz RingBlocks::read(void* buf, usz size) {
    usz ret = 0;
    RingBlocks::Block* block;
    u16 step = 0;
    while (ret < size) {
        block = getHeadBlock();
        if (!block) {
            break;
        }
        step = static_cast<u16>(AppMin<usz>(block->mUsed - block->mOffset, size - ret));
        memcpy(reinterpret_cast<s8*>(buf) + ret, block->mBuf + block->mOffset, step);
        block->mOffset += step;
        ret += step;
        if (block->mOffset == block->mUsed) {
            commitHead();
        }
    }
    return ret;
}

RingBlocks::Block* RingBlocks::getHeadBlock() {
    u32 head = mHead;
    u32 tail = getTail();
    u32 mask = mMask;
    if ((head & mask) == (tail & mask)) {
        return nullptr;
    }
    usz idx = head & mask;
    s8* ret = mCache + idx * mBlockSize;
    return reinterpret_cast<RingBlocks::Block*>(ret);
}

RingBlocks::Block* RingBlocks::getTailBlock() {
    u32 head = getHead();
    u32 tail = mTail;
    u32 mask = mMask;
    if (((tail + 1) & mask) == (head & mask)) {
        return nullptr;
    }
    usz idx = tail & mask;
    s8* ret = mCache + idx * mBlockSize;
    return reinterpret_cast<RingBlocks::Block*>(ret);
}

void RingBlocks::commitHead() {
    std::atomic_store_explicit(reinterpret_cast<std::atomic<u32>*>(&mHead), mHead + 1, std::memory_order_release);
}

void RingBlocks::commitTail() {
    std::atomic_store_explicit(reinterpret_cast<std::atomic<u32>*>(&mTail), mTail + 1, std::memory_order_release);
}

u32 RingBlocks::getHead() const {
    u32 ret = std::atomic_load_explicit(reinterpret_cast<const std::atomic<u32>*>(&mHead), std::memory_order_acquire);
    return ret;
}

u32 RingBlocks::getTail() const {
    u32 ret = std::atomic_load_explicit(reinterpret_cast<const std::atomic<u32>*>(&mTail), std::memory_order_acquire);
    return ret;
}

u32 RingBlocks::getBlockSize() const {
    return mBlockSize;
}


} // namespace app