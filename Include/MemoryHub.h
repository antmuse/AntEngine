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


#ifndef ANT_MEMORYHUB_H
#define ANT_MEMORYHUB_H

#include <mutex>
#include <atomic>
#include "MemoryPool.h"

#define D_THREADSAFE_MEMPOOL

namespace app {

///allocate some number of bytes from pools.  Uses the heap if necessary.
class MemoryHub {
public:
    enum EMemType {
        EMT_128,
        EMT_256,
        EMT_512,
        EMT_1024,
        EMT_2048,
        EMT_4096,
        EMT_8192,
        EMT_10K,
        EMT_DEFAULT
    };

    MemoryHub();

    ~MemoryHub();

    void setPageCount(s32 count);

    //@param align 对齐单位(必须是2的幂次方)
    s8* allocate(u64 bytesWanted, u64 align = sizeof(void*));

    //@param align 对齐单位(必须是2的幂次方)
    s8* allocateAndClear(u64 bytesWanted, u64 align = sizeof(void*));

    void release(void* data);

    void clear();

    void grab();

    void drop();

protected:
    
    struct SMemHead {
        u8 mHeadSize;
        u8 mMemTypte; //@see EMemType
    };
    CMemoryPool128 mPool128;
    CMemoryPool256 mPool256;
    CMemoryPool512 mPool512;
    CMemoryPool1024 mPool1024;
    CMemoryPool2048 mPool2048;
    CMemoryPool4096 mPool4096;
    CMemoryPool8192 mPool8192;
    CMemoryPool10K mPool10K;

#ifdef D_THREADSAFE_MEMPOOL
    std::mutex mMutex128;
    std::mutex mMutex256;
    std::mutex mMutex512;
    std::mutex mMutex1024;
    std::mutex mMutex2048;
    std::mutex mMutex4096;
    std::mutex mMutex8192;
    std::mutex mMutex10K;
#endif

private:
    std::atomic<s64> mReferenceCount;

    MemoryHub(const MemoryHub&) = delete;
    MemoryHub(const MemoryHub&&) = delete;
    MemoryHub& operator=(const MemoryHub&) = delete;
    MemoryHub& operator=(const MemoryHub&&) = delete;

    DFINLINE s8* getUserPointer(s8* real, const u64 align, const EMemType tp)const {
        s8* user = AppAlignPoint(real, align);
        user = user >= real + sizeof(SMemHead) ? user : user + align;
        SMemHead& hd = *reinterpret_cast<SMemHead*>(user - sizeof(SMemHead));
        hd.mHeadSize = (u8) (user - real);
        hd.mMemTypte = tp;
        return user;
    }

    DFINLINE const EMemType getRealPointer(const void* user, s8*& real)const {
        const SMemHead& hd = *(reinterpret_cast<const SMemHead*>(((s8*) user) - sizeof(SMemHead)));
        real = ((s8*) user) - hd.mHeadSize;
        return (EMemType) hd.mMemTypte;
    }
};

}//namespace app


#endif //ANT_MEMORYHUB_H

