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

#include <atomic>
#include "MemoryPool.h"
#include "Logger.h"


#define MINFRAGMENT 64

namespace app {
static s16 AppGenPoolID() {
    static std::atomic<s16> ids(0x1A10);
    return ++ids;
}

MemPool* MemPool::createMemPool(s32 psz) {
    psz += sizeof(MemPool);
    MemPool* ret = (MemPool*)malloc(psz);
    new (ret) MemPool();
    ret->initPool(psz);
    return ret;
}

void MemPool::releaseMemPool(MemPool* it) {
    it->~MemPool();
    free(it);
}

MemPool::MemPool() : mTotal(0), mUsed(0), mPoolID(AppGenPoolID()), mRover(nullptr) {
}

MemPool::~MemPool() {
}

void MemPool::initPool(s32 total) {
    std::lock_guard<std::mutex> ak(mMutex);
    MemBlock* block = (MemBlock*)((s8*)this + sizeof(MemPool));

    // set the entire zone to one free block
    mBlockList.mNext = block;
    mBlockList.mPrev = block;
    mBlockList.mTag = TAG_MEM_POOL; // in use block
    mBlockList.mID = 0;
    mBlockList.mSize = 0;
    mRover = block;
    mTotal = total;
    mUsed = 0;

    block->mPrev = block->mNext = &mBlockList;
    block->mTag = TAG_FREE; // free block
    block->mID = mPoolID;
    block->mSize = mTotal - sizeof(MemPool);
}

s32 MemPool::getAvailable() {
    return mTotal - mUsed;
}

void MemPool::release(void* ptr) {
    if (!ptr) {
        Logger::logError("MemPool::release: NULL pointer");
    }
    std::lock_guard<std::mutex> ak(mMutex);

    MemBlock* block = (MemBlock*)((s8*)ptr - sizeof(MemBlock));
    if (block->mID != mPoolID) {
        Logger::logError("MemPool::release: freed a pointer without mPoolID");
    }
    if (block->mTag == TAG_FREE) {
        Logger::logError("MemPool::release: freed a freed pointer");
    }
    if (block->mTag == TAG_MEM_NEW) {
        block->mTag = TAG_FREE;
        free(block);
        return;
    }

    // check the memory trash tester
    if (*(s32*)((s8*)block + block->mSize - 4) != mPoolID) {
        Logger::logError("MemPool::release: memory block wrote past end");
    }

    mUsed -= block->mSize;

    // set the block to something that should cause problems if it is referenced...
    memset(ptr, 0xAA, block->mSize - sizeof(*block));

    block->mTag = TAG_FREE; // mark as free
    MemBlock* other = block->mPrev;
    if (!other->mTag) {
        // merge with previous free block
        other->mSize += block->mSize;
        other->mNext = block->mNext;
        other->mNext->mPrev = other;
        if (block == mRover) {
            mRover = other;
        }
        block = other;
    }

    mRover = block;

    other = block->mNext;
    if (!other->mTag) {
        // merge the next free block onto the end
        block->mSize += other->mSize;
        block->mNext = other->mNext;
        block->mNext->mPrev = block;
    }
}


void MemPool::releaseTags(s32 tag) {
    std::lock_guard<std::mutex> ak(mMutex);

    // use the mRover as our pointer, because release automatically adjusts it
    mRover = mBlockList.mNext;
    do {
        if (mRover->mTag == tag) {
            release((void*)(mRover + 1));
            continue;
        }
        mRover = mRover->mNext;
    } while (mRover != &mBlockList);
}


#ifdef DDEBUG
s8* MemPool::allocate(s32 wsize, const s8* label, const s8* file, s32 line) {
    s32 allocSize;
#else
s8* MemPool::allocate(s32 wsize) {
#endif
    s32 extra;
    MemBlock *start, *tmp_rover, *newblock, *base;
#ifdef DDEBUG
    allocSize = wsize;
#endif

    // scan through the block list looking for the first free block of sufficient size
    wsize += sizeof(MemBlock);         // account for size of block header
    wsize += sizeof(s32);              // space for memory trash tester
    wsize = AppAlignSize(wsize, sizeof(void*)); // align to 32/64 bit boundary

    std::lock_guard<std::mutex> ak(mMutex);

    base = tmp_rover = mRover;
    start = base->mPrev;

    do {
        if (tmp_rover == start) {
            // scaned all the way around the list
#ifdef DDEBUG
            Logger::logError("MemPool::allocate: failed on allocation of %d bytes from the pool: %s, line: %d (%s)",
                wsize, file, line, label);
#else
            Logger::logError("MemPool::allocate: failed on allocation of %d bytes from the pool", wsize);
#endif
            MemBlock* ret = (MemBlock*)malloc(wsize);
            ret->mSize = wsize;
            ret->mID = mPoolID;
            ret->mTag = TAG_MEM_NEW;
            ret->mNext = nullptr;
            ret->mPrev = nullptr;
#ifdef DDEBUG
            ret->mDebugInfo.mLabel = label;
            ret->mDebugInfo.mFile = file;
            ret->mDebugInfo.mLine = line;
            ret->mDebugInfo.mAllocSize = allocSize;
#endif
            return ((s8*)ret + sizeof(MemBlock));
        }
        if (tmp_rover->mTag) {
            base = tmp_rover = tmp_rover->mNext;
        } else {
            tmp_rover = tmp_rover->mNext;
        }
    } while (base->mTag || base->mSize < wsize);

    // found a block big enough
    extra = base->mSize - wsize;
    if (extra > MINFRAGMENT) {
        // there will be a free fragment after the allocated block
        newblock = (MemBlock*)((s8*)base + wsize);
        newblock->mSize = extra;
        newblock->mTag = 0; // free block
        newblock->mPrev = base;
        newblock->mID = mPoolID;
        newblock->mNext = base->mNext;
        newblock->mNext->mPrev = newblock;
        base->mNext = newblock;
        base->mSize = wsize;
    }

    base->mTag = TAG_MEM_POOL; // no longer a free block

    mRover = base->mNext; // next allocation will start looking here
    mUsed += base->mSize;

    base->mID = mPoolID;

#ifdef DDEBUG
    base->mDebugInfo.mLabel = label;
    base->mDebugInfo.mFile = file;
    base->mDebugInfo.mLine = line;
    base->mDebugInfo.mAllocSize = allocSize;
#endif

    // marker for memory trash testing
    *(s32*)((s8*)base + base->mSize - 4) = mPoolID;

    return ((s8*)base + sizeof(MemBlock));
}


#ifdef DDEBUG
s8* MemPool::allocateAndClear(s32 wsize, const s8* label, const s8* file, s32 line) {
    checkPool(); // DEBUG
    s8* ret = allocate(wsize, label, file, line);
#else
s8* MemPool::allocateAndClear(s32 wsize) {
    s8* ret = allocate(wsize);
#endif
    memset(ret, 0, wsize);
    return ret;
}


void MemPool::checkPool() {
    std::lock_guard<std::mutex> ak(mMutex);

    for (MemBlock* block = mBlockList.mNext;; block = block->mNext) {
        if (block->mNext == &mBlockList) {
            break; // all blocks have been hit
        }
        if ((s8*)block + block->mSize != (s8*)block->mNext)
            Logger::logError("MemPool::checkPool: block size does not touch the next block");
        if (block->mNext->mPrev != block) {
            Logger::logError("MemPool::checkPool: next block doesn't have proper back link");
        }
        if (!block->mTag && !block->mNext->mTag) {
            Logger::logError("MemPool::checkPool: two consecutive free blocks");
        }
    }
}


void MemPool::logPool(const s8* name) {
    std::lock_guard<std::mutex> ak(mMutex);

    name = name ? name : "MemPool";
#ifdef DDEBUG
    s8 dump[32], *ptr;
    s32 i, j;
#endif
    s32 allocSize = 0;
    s8 buf[4096];
    s32 tsize = 0;
    s32 numBlocks = 0;
    MemBlock* block;

    snprintf(buf, sizeof(buf), "\r\n================\r\n%s log\r\n================\r\n", name);
    Logger::logInfo(buf);
    for (block = mBlockList.mNext; block->mNext != &mBlockList; block = block->mNext) {
        if (block->mTag) {
#ifdef DDEBUG
            ptr = ((s8*)block) + sizeof(MemBlock);
            j = 0;
            for (i = 0; i < 20 && i < block->mDebugInfo.mAllocSize; i++) {
                if (ptr[i] >= 32 && ptr[i] < 127) {
                    dump[j++] = ptr[i];
                } else {
                    dump[j++] = '_';
                }
            }
            dump[j] = '\0';
            snprintf(buf, sizeof(buf), "size=%8d, %s:%d, %s, dump[%s]\r\n", block->mDebugInfo.mAllocSize,
                block->mDebugInfo.mFile, block->mDebugInfo.mLine, block->mDebugInfo.mLabel, dump);
            Logger::logInfo(buf);
            allocSize += block->mDebugInfo.mAllocSize;
#endif
            tsize += block->mSize;
            numBlocks++;
        }
    }
#ifdef DDEBUG
    // subtract debug memory
    tsize -= numBlocks * sizeof(MemDebug);
#else
    allocSize = numBlocks * sizeof(MemBlock); // + 32 bit alignment
#endif
    snprintf(buf, sizeof(buf), "%s: %d,%d/%d memory in %d blocks\r\n", name, tsize, mUsed, mTotal, numBlocks);
    Logger::logInfo(buf);
    snprintf(buf, sizeof(buf), "%s: %d memory overhead\r\n", name, tsize - allocSize);
    Logger::logInfo(buf);
}


} // namespace app
