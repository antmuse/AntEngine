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


#include "MemoryHub.h"
#include <memory.h>

namespace app {

MemoryHub::MemoryHub() :
    mReferenceCount(1) {
    setPageCount(16);
}


MemoryHub::~MemoryHub() {
    DASSERT(0 == mReferenceCount.load());
}


void MemoryHub::grab() {
    DASSERT(mReferenceCount > 0);
    ++mReferenceCount;
}


void MemoryHub::drop() {
    s64 ret = --mReferenceCount;
    DASSERT(ret >= 0);
    if (0 == ret) {
        delete this;
    }
}


void MemoryHub::setPageCount(s32 cnt) {
    mPool128.setPageSize(128 * cnt);
    mPool256.setPageSize(256 * cnt);
    mPool512.setPageSize(512 * cnt);
    mPool1024.setPageSize(1024 * cnt);
    mPool2048.setPageSize(2048 * cnt);
    mPool4096.setPageSize(4096 * cnt);
    mPool8192.setPageSize(8192 * cnt);
    mPool10K.setPageSize(10240 * cnt);
}

s8* MemoryHub::allocateAndClear(u64 bytesWanted, u64 align/* = sizeof(void*)*/) {
    s8* ret = allocate(bytesWanted, align);
    memset(ret, 0, bytesWanted);
    return ret;
}

s8* MemoryHub::allocate(u64 bytesWanted, u64 align/* = sizeof(void*)*/) {
#if defined(DDEBUG)
    grab();
#endif

    align = align < sizeof(SMemHead) ? sizeof(SMemHead) : align;
    bytesWanted += align;
    bytesWanted = AppAlignSize(bytesWanted, align);
#ifdef APP_DISABLE_BYTE_POOL
    return malloc(bytesWanted);
#endif
    s8* out;
    if (bytesWanted <= 128) {
#ifdef D_THREADSAFE_MEMPOOL
        mMutex128.lock();
#endif
        out = (s8*)mPool128.allocate();
#ifdef D_THREADSAFE_MEMPOOL
        mMutex128.unlock();
#endif
        return getUserPointer(out, align, EMT_128);
    }
    if (bytesWanted <= 256) {
#ifdef D_THREADSAFE_MEMPOOL
        mMutex256.lock();
#endif
        out = (s8*)mPool256.allocate();
#ifdef D_THREADSAFE_MEMPOOL
        mMutex256.unlock();
#endif
        return getUserPointer(out, align, EMT_256);
    }
    if (bytesWanted <= 512) {
#ifdef D_THREADSAFE_MEMPOOL
        mMutex512.lock();
#endif
        out = (s8*)mPool512.allocate();
#ifdef D_THREADSAFE_MEMPOOL
        mMutex512.unlock();
#endif
        return getUserPointer(out, align, EMT_512);
    }
    if (bytesWanted <= 1024) {
#ifdef D_THREADSAFE_MEMPOOL
        mMutex1024.lock();
#endif
        out = (s8*)mPool1024.allocate();
#ifdef D_THREADSAFE_MEMPOOL
        mMutex1024.unlock();
#endif
        return getUserPointer(out, align, EMT_1024);
    }
    if (bytesWanted <= 2048) {
#ifdef D_THREADSAFE_MEMPOOL
        mMutex2048.lock();
#endif
        out = (s8*)mPool2048.allocate();
#ifdef D_THREADSAFE_MEMPOOL
        mMutex2048.unlock();
#endif
        return getUserPointer(out, align, EMT_2048);
    }
    if (bytesWanted <= 4096) {
#ifdef D_THREADSAFE_MEMPOOL
        mMutex4096.lock();
#endif
        out = (s8*)mPool4096.allocate();
#ifdef D_THREADSAFE_MEMPOOL
        mMutex4096.unlock();
#endif
        return getUserPointer(out, align, EMT_4096);
    }
    if (bytesWanted <= 8192) {
#ifdef D_THREADSAFE_MEMPOOL
        mMutex8192.lock();
#endif
        out = (s8*)mPool8192.allocate();
#ifdef D_THREADSAFE_MEMPOOL
        mMutex8192.unlock();
#endif
        return getUserPointer(out, align, EMT_8192);
    }
    if (bytesWanted <= 10240) {
#ifdef D_THREADSAFE_MEMPOOL
        mMutex10K.lock();
#endif
        out = (s8*)mPool10K.allocate();
#ifdef D_THREADSAFE_MEMPOOL
        mMutex10K.unlock();
#endif
        return getUserPointer(out, align, EMT_10K);
    }
    out = (s8*) ::malloc(bytesWanted + 1);
    return getUserPointer(out, align, EMT_DEFAULT);
}


void MemoryHub::release(void* data) {
    if (!data) {
        return;
    }
#if defined(DDEBUG)
    drop();
#endif

#ifdef APP_DISABLE_BYTE_POOL
    ::free(data);
#endif
    s8* realData;
    switch (getRealPointer(data, realData)) {
    case EMT_128:
#ifdef D_THREADSAFE_MEMPOOL
        mMutex128.lock();
#endif
        mPool128.release((u8(*)[128]) realData);
#ifdef D_THREADSAFE_MEMPOOL
        mMutex128.unlock();
#endif
        break;
    case EMT_256:
#ifdef D_THREADSAFE_MEMPOOL
        mMutex256.lock();
#endif
        mPool256.release((u8(*)[256]) realData);
#ifdef D_THREADSAFE_MEMPOOL
        mMutex256.unlock();
#endif
        break;
    case EMT_512:
#ifdef D_THREADSAFE_MEMPOOL
        mMutex512.lock();
#endif
        mPool512.release((u8(*)[512]) realData);
#ifdef D_THREADSAFE_MEMPOOL
        mMutex512.unlock();
#endif
        break;
    case EMT_1024:
#ifdef D_THREADSAFE_MEMPOOL
        mMutex1024.lock();
#endif
        mPool1024.release((u8(*)[1024]) realData);
#ifdef D_THREADSAFE_MEMPOOL
        mMutex1024.unlock();
#endif
        break;
    case EMT_2048:
#ifdef D_THREADSAFE_MEMPOOL
        mMutex2048.lock();
#endif
        mPool2048.release((u8(*)[2048]) realData);
#ifdef D_THREADSAFE_MEMPOOL
        mMutex2048.unlock();
#endif
        break;
    case EMT_4096:
#ifdef D_THREADSAFE_MEMPOOL
        mMutex4096.lock();
#endif
        mPool4096.release((u8(*)[4096]) realData);
#ifdef D_THREADSAFE_MEMPOOL
        mMutex4096.unlock();
#endif
        break;
    case EMT_8192:
#ifdef D_THREADSAFE_MEMPOOL
        mMutex8192.lock();
#endif
        mPool8192.release((u8(*)[8192]) realData);
#ifdef D_THREADSAFE_MEMPOOL
        mMutex8192.unlock();
#endif
        break;
    case EMT_10K:
#ifdef D_THREADSAFE_MEMPOOL
        mMutex10K.lock();
#endif
        mPool10K.release((u8(*)[10240]) realData);
#ifdef D_THREADSAFE_MEMPOOL
        mMutex10K.unlock();
#endif
        break;
    case EMT_DEFAULT:
        ::free(realData);
        break;
    default:
        DASSERT(0);
        break;
    }
}


void MemoryHub::clear() {
#ifdef D_THREADSAFE_MEMPOOL
    mPool128.clear();
    mPool256.clear();
    mPool512.clear();
    mPool1024.clear();
    mPool2048.clear();
    mPool4096.clear();
    mPool8192.clear();
    mPool10K.clear();
#endif //D_THREADSAFE_MEMPOOL
}

}//namespace app
