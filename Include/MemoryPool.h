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


#ifndef APP_MEMORYPOOL_H
#define APP_MEMORYPOOL_H

#include <stdlib.h>
#include <mutex>
#include "Config.h"

// D_MEMPOOL_MAX_PAGE must be > 1
#define D_MEMPOOL_MAX_PAGE 4

//#define D_DISABLE_MEMPOOL

namespace app {

/**
* @brief Very fast memory pool for allocating and deallocating structures that don't have constructors or destructors.
* Contains a list of pages, each of which has an array of the user structures
*/
template <class MemoryBlockType>
class MemoryPool {
public:
    struct SMemoryPage;
    struct SMemoryWithPage {
        MemoryBlockType mUserMemory;
        SMemoryPage* mParentPage;
    };

    struct SMemoryPage {
        SMemoryWithPage** mAvailableStack;
        s32 mAvailableStackSize;
        SMemoryWithPage* mBlock;
        SMemoryPage* mNext;
        SMemoryPage* mPrevious;
    };

    MemoryPool();

    ~MemoryPool();

    static s32 getBlockSize() {
        return sizeof(MemoryBlockType);
    }

    void setPageSize(s32 size); // Defaults to 16*1024 bytes

    MemoryBlockType* allocate();

    void release(MemoryBlockType* it);

    void clear();

    s32 getAvailablePagesSize(void) const {
        return mAvailablePagesSize;
    }

    s32 getUnavailablePagesSize(void) const {
        return mUnavailablePagesSize;
    }

    s32 getMemoryPoolPageSize(void) const {
        return mMemoryPoolPageSize;
    }

protected:
    s32 blocksPerPage(void) const;

    //void allocateFirst(void);

    bool initPage(SMemoryPage* page, SMemoryPage* iPrevious);

    // mAvailablePages contains pages which have room to give the user new blocks.  We return these blocks from the head of the list
    // mUnavailablePages are pages which are totally full, and from which we do not return new blocks.
    // Pages move from the head of mUnavailablePages to the tail of mAvailablePages, and from the head of mAvailablePages to the tail of mUnavailablePages
    SMemoryPage* mAvailablePages;
    SMemoryPage* mUnavailablePages;
    s32 mAvailablePagesSize;
    s32 mUnavailablePagesSize;
    s32 mMemoryPoolPageSize;
};


template<class MemoryBlockType>
MemoryPool<MemoryBlockType>::MemoryPool() {
#ifndef D_DISABLE_MEMPOOL
    //allocateFirst();
    mAvailablePagesSize = 0;
    mUnavailablePagesSize = 0;
    setPageSize(16 * 1024);
#endif
}


template<class MemoryBlockType>
MemoryPool<MemoryBlockType>::~MemoryPool() {
#ifndef D_DISABLE_MEMPOOL
    clear();
#endif
}


template<class MemoryBlockType>
void MemoryPool<MemoryBlockType>::setPageSize(s32 size) {
    s32 count = (size + sizeof(SMemoryWithPage) - 1) / sizeof(SMemoryWithPage);
    mMemoryPoolPageSize = count * sizeof(SMemoryWithPage);
}


template<class MemoryBlockType>
MemoryBlockType* MemoryPool<MemoryBlockType>::allocate() {
#ifdef D_DISABLE_MEMPOOL
    return (MemoryBlockType*) ::malloc(sizeof(MemoryBlockType));
#else

    if(mAvailablePagesSize > 0) {
        SMemoryPage *curPage = mAvailablePages;
        MemoryBlockType *retVal = (MemoryBlockType*) curPage->mAvailableStack[--(curPage->mAvailableStackSize)];
        if(curPage->mAvailableStackSize == 0) {
            --mAvailablePagesSize;
            mAvailablePages = curPage->mNext;
            DASSERT(mAvailablePagesSize == 0 || mAvailablePages->mAvailableStackSize > 0);
            curPage->mNext->mPrevious = curPage->mPrevious;
            curPage->mPrevious->mNext = curPage->mNext;

            if(mUnavailablePagesSize++ == 0) {
                mUnavailablePages = curPage;
                curPage->mNext = curPage;
                curPage->mPrevious = curPage;
            } else {
                curPage->mNext = mUnavailablePages;
                curPage->mPrevious = mUnavailablePages->mPrevious;
                mUnavailablePages->mPrevious->mNext = curPage;
                mUnavailablePages->mPrevious = curPage;
            }
        }

        DASSERT(mAvailablePagesSize == 0 || mAvailablePages->mAvailableStackSize > 0);
        return retVal;
    }//if

    mAvailablePages = (SMemoryPage *) ::malloc(sizeof(SMemoryPage));
    if(mAvailablePages == 0) {
        return 0;
    }
    mAvailablePagesSize = 1;

    if(initPage(mAvailablePages, mAvailablePages) == false) {
        return 0;
    }
    // If this assert hits, we couldn't allocate even 1 mBlock per page. Increase the page size
    DASSERT(mAvailablePages->mAvailableStackSize > 1);

    return (MemoryBlockType*) mAvailablePages->mAvailableStack[--mAvailablePages->mAvailableStackSize];
#endif
}


template<class MemoryBlockType>
void MemoryPool<MemoryBlockType>::release(MemoryBlockType *m) {
#ifdef D_DISABLE_MEMPOOL
    ::free(m);
    return;
#else
    // Find the page this mBlock is in and return it.
    SMemoryWithPage *memoryWithPage = (SMemoryWithPage*) m;
    SMemoryPage *curPage = memoryWithPage->mParentPage;

    if(curPage->mAvailableStackSize == 0) {
        // The page is in the unavailable list so move it to the available list
        curPage->mAvailableStack[curPage->mAvailableStackSize++] = memoryWithPage;
        mUnavailablePagesSize--;

        // As this page is no longer totally empty, move it to the end of available pages
        curPage->mNext->mPrevious = curPage->mPrevious;
        curPage->mPrevious->mNext = curPage->mNext;

        if(mUnavailablePagesSize > 0 && curPage == mUnavailablePages)
            mUnavailablePages = mUnavailablePages->mNext;

        if(mAvailablePagesSize++ == 0) {
            mAvailablePages = curPage;
            curPage->mNext = curPage;
            curPage->mPrevious = curPage;
        } else {
            curPage->mNext = mAvailablePages;
            curPage->mPrevious = mAvailablePages->mPrevious;
            mAvailablePages->mPrevious->mNext = curPage;
            mAvailablePages->mPrevious = curPage;
        }
    } else {
        curPage->mAvailableStack[curPage->mAvailableStackSize++] = memoryWithPage;

        if(curPage->mAvailableStackSize == blocksPerPage() &&
            mAvailablePagesSize >= D_MEMPOOL_MAX_PAGE) {
            // After a certain point, just deallocate empty pages rather than keep them around
            if(curPage == mAvailablePages) {
                mAvailablePages = curPage->mNext;
                DASSERT(mAvailablePages->mAvailableStackSize > 0);
            }
            curPage->mPrevious->mNext = curPage->mNext;
            curPage->mNext->mPrevious = curPage->mPrevious;
            mAvailablePagesSize--;
            ::free(curPage->mAvailableStack);
            ::free(curPage->mBlock);
            ::free(curPage);
        }
    }
#endif
}


template<class MemoryBlockType>
void MemoryPool<MemoryBlockType>::clear() {
#ifdef D_DISABLE_MEMPOOL
    return;
#else
    SMemoryPage *cur, *freed;

    if(mAvailablePagesSize > 0) {
        cur = mAvailablePages;
#ifdef _MSC_VER
#pragma warning(disable:4127)   // conditional expression is constant
#endif
        while(true) {
            free(cur->mAvailableStack);
            free(cur->mBlock);
            freed = cur;
            cur = cur->mNext;
            if(cur == mAvailablePages) {
                free(freed);
                break;
            }
            free(freed);
        }// while
    }//if

    if(mUnavailablePagesSize > 0) {
        cur = mUnavailablePages;
        while(1) {
            free(cur->mAvailableStack);
            free(cur->mBlock);
            freed = cur;
            cur = cur->mNext;
            if(cur == mUnavailablePages) {
                free(freed);
                break;
            }
            free(freed);
        } // while
    }//if

    mAvailablePagesSize = 0;
    mUnavailablePagesSize = 0;
#endif
}


template<class MemoryBlockType>
s32 MemoryPool<MemoryBlockType>::blocksPerPage(void) const {
    return mMemoryPoolPageSize / sizeof(SMemoryWithPage);
}


template<class MemoryBlockType>
bool MemoryPool<MemoryBlockType>::initPage(SMemoryPage* page, SMemoryPage* iPrevious) {
    const s32 bpp = blocksPerPage();
    page->mBlock = (SMemoryWithPage*) ::malloc(mMemoryPoolPageSize);

    if(page->mBlock == 0) {
        return false;
    }
    page->mAvailableStack = (SMemoryWithPage**) ::malloc(sizeof(SMemoryWithPage*)*bpp);
    if(page->mAvailableStack == 0) {
        ::free(page->mBlock);
        return false;
    }
    SMemoryWithPage *curBlock = page->mBlock;
    SMemoryWithPage **curStack = page->mAvailableStack;
    s32 i = 0;
    while(i < bpp) {
        curBlock->mParentPage = page;
        curStack[i] = curBlock++;
        i++;
    }
    page->mAvailableStackSize = bpp;
    page->mNext = mAvailablePages;
    page->mPrevious = iPrevious;
    return true;
}


typedef MemoryPool<u8[128]> CMemoryPool128;
typedef MemoryPool<u8[256]> CMemoryPool256;
typedef MemoryPool<u8[512]> CMemoryPool512;
typedef MemoryPool<u8[1024]> CMemoryPool1024;
typedef MemoryPool<u8[2048]> CMemoryPool2048;
typedef MemoryPool<u8[4096]> CMemoryPool4096;
typedef MemoryPool<u8[8192]> CMemoryPool8192;
typedef MemoryPool<u8[10240]> CMemoryPool10K;






/**
 * @brief MemPool is ZONE MEMORY ALLOCATION
 * There is never any space between memblocks, and there will never be two
 * contiguous free memblocks.
 * The mRover can be left pointing at a non-empty block
 * The zone calls are pretty much only mUsed for small strings and structures,
 * all big things are allocated on the hunk.
 */
class MemPool {
public:
    static MemPool* createMemPool(s32 psz);
    static void releaseMemPool(MemPool* it);
    MemPool(const MemPool&) = delete;
    MemPool(const MemPool&&) = delete;
    MemPool& operator=(const MemPool&) = delete;
    MemPool& operator=(const MemPool&&) = delete;

    void releaseTags(s32 tag);
#ifdef DDEBUG
    s8* allocate(s32 wsize, const s8* label = "lab", const s8* file = __FILE__, s32 line = __LINE__);
    s8* allocateAndClear(s32 wsize, const s8* label = "lab", const s8* file = __FILE__, s32 line = __LINE__);
#else
    s8* allocate(s32 wsize);
    s8* allocateAndClear(s32 wsize);
#endif

    void release(void* val);

    void initPool(s32 total);
    void logPool(const s8* name = "MemPool");
    void checkPool();
    s32 getAvailable();

private:
    MemPool();
    ~MemPool();

    enum EMemTag {
        TAG_FREE = 0,   // free tag, must=0
        TAG_MEM_POOL,
        TAG_MEM_NEW
    };

    struct MemDebug {
        const s8* mLabel;
        const s8* mFile;
        s32 mLine;
        s32 mAllocSize;
    };

    struct MemBlock {
        s32 mSize; // including the header and possibly tiny fragments
        s16 mTag;  // a tag of 0 is a free block
        s16 mID;   // should be pool's ID
        MemBlock* mNext;
        MemBlock* mPrev;
#ifdef DDEBUG
        MemDebug mDebugInfo;
#endif
    };
    s32 mTotal;            // total bytes malloced, including header
    s32 mUsed;             // total bytes used
    s16 mPoolID;           // pool's ID
    s16 mPad;              // reserve
    std::mutex mMutex;
    MemBlock mBlockList;   // start/end cap for linked list
    MemBlock* mRover;
};

}//namespace app


#endif // APP_MEMORYPOOL_H
