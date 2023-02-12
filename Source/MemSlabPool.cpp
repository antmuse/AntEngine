#include <memory>
#include "MemSlabPool.h"
#include "Logger.h"
#include "System.h"

namespace app {

#define D_SLAB_PAGE_MASK   3
#define D_SLAB_PAGE        0
#define D_SLAB_BIG         1
#define D_SLAB_EXACT       2
#define D_SLAB_SMALL       3

#if defined(DOS_32BIT)

#define D_SLAB_PAGE_FREE   0
#define D_SLAB_PAGE_BUSY   0xffffffff
#define D_SLAB_PAGE_START  0x80000000

#define D_SLAB_SHIFT_MASK  0x0000000f
#define D_SLAB_MAP_MASK    0xffff0000
#define D_SLAB_MAP_SHIFT   16

#define D_SLAB_BUSY        0xffffffff

#else //64bit sizeof(usz) == 8

#define D_SLAB_PAGE_FREE   0
#define D_SLAB_PAGE_BUSY   0xffffffffffffffff
#define D_SLAB_PAGE_START  0x8000000000000000

#define D_SLAB_SHIFT_MASK  0x000000000000000f
#define D_SLAB_MAP_MASK    0xffffffff00000000
#define D_SLAB_MAP_SHIFT   32

#define D_SLAB_BUSY        0xffffffffffffffff

#endif


#define DGET_PAGE_TYPE(page)   ((page)->mPrev & D_SLAB_PAGE_MASK)

#define DGET_PAGE_PREV(page)   (MemPage*)((page)->mPrev & ~D_SLAB_PAGE_MASK)


#if (D_DEBUG_SLAB)
#define DSLAB_JUNK(p, size)  memset(p, 0xA5, size)
#else
#define DSLAB_JUNK(p, size)
#endif

static usz G_MaxSlabSize;
static usz G_ExactSlabSize;
static usz G_ExactSlabSizeShift = 0;

static usz G_PageSize = System::getPageSize();
static usz G_PageSizeShift = 0;



MemSlabPool::MemSlabPool(usz msz) {
    mTotalSize = msz;
    mEndPos = (u8*)this + msz;
    mMinShift = 3;
    initSlabSize();
    initSlab();
}

void MemSlabPool::initSlabSize() {
    G_PageSize = System::getPageSize();
    for (usz i = G_PageSize;
        i >>= 1; G_PageSizeShift++) {
        //
    }
    G_ExactSlabSizeShift = 0;
    G_MaxSlabSize = G_PageSize / 2;
    G_ExactSlabSize = G_PageSize / (8 * sizeof(usz));
    for (usz n = G_ExactSlabSize; n >>= 1; G_ExactSlabSizeShift++) {
    }
}


void MemSlabPool::initSlab() {
    u8* p;
    usz size;
    s64 m;
    usz i, n, pages;
    MemPage* slots, *page;

    mMinSize = (usz)1 << mMinShift;

    slots = getSlots();

    p = (u8*)slots;
    size = mEndPos - p;

    DSLAB_JUNK(p, size);

    n = G_PageSizeShift - mMinShift;

    for (i = 0; i < n; i++) {
        // only "next" is used in list head
        slots[i].mSlab = 0;
        slots[i].mNext = &slots[i];
        slots[i].mPrev = 0;
    }

    p += n * sizeof(MemPage);

    mStats = (MemStat*)p;
    memset(mStats, 0, n * sizeof(MemStat));

    p += n * sizeof(MemStat);

    size -= n * (sizeof(MemPage) + sizeof(MemStat));

    pages = (usz)(size / (G_PageSize + sizeof(MemPage)));

    mPages = (MemPage*)p;
    memset(mPages, 0, pages * sizeof(MemPage));

    page = mPages;

    // only "next" is used in list head
    mFreePage.mSlab = 0;
    mFreePage.mNext = page;
    mFreePage.mPrev = 0;

    page->mSlab = pages;
    page->mNext = &mFreePage;
    page->mPrev = (usz)&mFreePage;

    mStartPos = AppAlignPoint(p + pages * sizeof(MemPage),
        G_PageSize);

    m = pages - (mEndPos - mStartPos) / G_PageSize;
    if (m > 0) {
        pages -= m;
        page->mSlab = pages;
    }

    mLastPage = mPages + pages;
    mFreePageCount = pages;

    this->mLogNoMem = 1;
}

u32 MemSlabPool::getStateCount() {
    return G_PageSizeShift - mMinShift;
}

usz MemSlabPool::getPageAddr(MemPage* page) {
    return ((((page)-mPages) << G_PageSizeShift) + (usz)mStartPos);
}


void* MemSlabPool::allocMem(usz size) {
    mLock.lock();
    void* p = allocMemNolock(size);
    mLock.unlock();
    return p;
}


void* MemSlabPool::allocMemNolock(usz size) {
    usz s;
    usz p, m, mask, * bitmap;
    usz i, n, slot, shift, map;
    MemPage* page, *prev, *slots;

    if (size > G_MaxSlabSize) {
        page = allocPages((size >> G_PageSizeShift) + ((size % G_PageSize) ? 1 : 0));
        if (page) {
            p = getPageAddr(page);
        } else {
            p = 0;
        }
        goto done;
    }

    if (size > mMinSize) {
        shift = 1;
        for (s = size - 1; s >>= 1; shift++) { /* void */ }
        slot = shift - mMinShift;

    } else {
        shift = mMinShift;
        slot = 0;
    }

    mStats[slot].mRequests++;

    slots = getSlots();
    page = slots[slot].mNext;

    if (page->mNext != page) {

        if (shift < G_ExactSlabSizeShift) {

            bitmap = (usz*)getPageAddr(page);

            map = (G_PageSize >> shift) / (8 * sizeof(usz));

            for (n = 0; n < map; n++) {

                if (bitmap[n] != D_SLAB_BUSY) {

                    for (m = 1, i = 0; m; m <<= 1, i++) {
                        if (bitmap[n] & m) {
                            continue;
                        }

                        bitmap[n] |= m;

                        i = (n * 8 * sizeof(usz) + i) << shift;

                        p = (usz)bitmap + i;

                        mStats[slot].mUsed++;

                        if (bitmap[n] == D_SLAB_BUSY) {
                            for (n = n + 1; n < map; n++) {
                                if (bitmap[n] != D_SLAB_BUSY) {
                                    goto done;
                                }
                            }

                            prev = DGET_PAGE_PREV(page);
                            prev->mNext = page->mNext;
                            page->mNext->mPrev = page->mPrev;

                            page->mNext = NULL;
                            page->mPrev = D_SLAB_SMALL;
                        }

                        goto done;
                    }
                }
            }

        } else if (shift == G_ExactSlabSizeShift) {

            for (m = 1, i = 0; m; m <<= 1, i++) {
                if (page->mSlab & m) {
                    continue;
                }

                page->mSlab |= m;

                if (page->mSlab == D_SLAB_BUSY) {
                    prev = DGET_PAGE_PREV(page);
                    prev->mNext = page->mNext;
                    page->mNext->mPrev = page->mPrev;

                    page->mNext = NULL;
                    page->mPrev = D_SLAB_EXACT;
                }

                p = getPageAddr(page) + (i << shift);

                mStats[slot].mUsed++;

                goto done;
            }

        } else { /* shift > G_ExactSlabSizeShift */

            mask = ((usz)1 << (G_PageSize >> shift)) - 1;
            mask <<= D_SLAB_MAP_SHIFT;

            for (m = (usz)1 << D_SLAB_MAP_SHIFT, i = 0;
                m & mask;
                m <<= 1, i++) {
                if (page->mSlab & m) {
                    continue;
                }

                page->mSlab |= m;

                if ((page->mSlab & D_SLAB_MAP_MASK) == mask) {
                    prev = DGET_PAGE_PREV(page);
                    prev->mNext = page->mNext;
                    page->mNext->mPrev = page->mPrev;

                    page->mNext = NULL;
                    page->mPrev = D_SLAB_BIG;
                }

                p = getPageAddr(page) + (i << shift);

                mStats[slot].mUsed++;

                goto done;
            }
        }

        Logger::logError("allocMem(): page is busy");
    }

    page = allocPages(1);

    if (page) {
        if (shift < G_ExactSlabSizeShift) {
            bitmap = (usz*)getPageAddr(page);

            n = (G_PageSize >> shift) / ((1 << shift) * 8);

            if (n == 0) {
                n = 1;
            }

            /* "n" elements for bitmap, plus one requested */

            for (i = 0; i < (n + 1) / (8 * sizeof(usz)); i++) {
                bitmap[i] = D_SLAB_BUSY;
            }

            m = ((usz)1 << ((n + 1) % (8 * sizeof(usz)))) - 1;
            bitmap[i] = m;

            map = (G_PageSize >> shift) / (8 * sizeof(usz));

            for (i = i + 1; i < map; i++) {
                bitmap[i] = 0;
            }

            page->mSlab = shift;
            page->mNext = &slots[slot];
            page->mPrev = (usz)&slots[slot] | D_SLAB_SMALL;

            slots[slot].mNext = page;

            mStats[slot].mTotal += (G_PageSize >> shift) - n;

            p = getPageAddr(page) + (n << shift);

            mStats[slot].mUsed++;

            goto done;

        } else if (shift == G_ExactSlabSizeShift) {

            page->mSlab = 1;
            page->mNext = &slots[slot];
            page->mPrev = (usz)&slots[slot] | D_SLAB_EXACT;

            slots[slot].mNext = page;

            mStats[slot].mTotal += 8 * sizeof(usz);

            p = getPageAddr(page);

            mStats[slot].mUsed++;

            goto done;

        } else { /* shift > G_ExactSlabSizeShift */

            page->mSlab = ((usz)1 << D_SLAB_MAP_SHIFT) | shift;
            page->mNext = &slots[slot];
            page->mPrev = (usz)&slots[slot] | D_SLAB_BIG;

            slots[slot].mNext = page;

            mStats[slot].mTotal += G_PageSize >> shift;

            p = getPageAddr(page);

            mStats[slot].mUsed++;

            goto done;
        }
    }

    p = 0;

    mStats[slot].mFails++;

    done:

    //ngx_log_debug1(D_LOG_DEBUG_ALLOC, ngx_cycle->log, 0, "slab alloc: %p", (void*)p);

    return (void*)p;
}


void* MemSlabPool::callocMem(usz size) {
    mLock.lock();
    void* p = callocMemNolock(size);
    mLock.unlock();
    return p;
}


void* MemSlabPool::callocMemNolock(usz size) {
    void* p = allocMemNolock(size);
    if (p) {
        memset(p, 0, size);
    }
    return p;
}


void MemSlabPool::freeMem(void* p) {
    mLock.lock();
    freeMemNolock(p);
    mLock.unlock();
}


void MemSlabPool::freeMemNolock(void* p) {
    usz            size;
    usz         slab, m, * bitmap;
    usz        i, n, type, slot, shift, map;
    MemPage* slots, * page;

    //ngx_log_debug1(D_LOG_DEBUG_ALLOC, ngx_cycle->log, 0, "slab free: %p", p);

    if ((u8*)p < mStartPos || (u8*)p > mEndPos) {
        Logger::logError("freeMem(): outside of pool");
        goto fail;
    }

    n = ((u8*)p - mStartPos) >> G_PageSizeShift;
    page = &mPages[n];
    slab = page->mSlab;
    type = DGET_PAGE_TYPE(page);

    switch (type) {

    case D_SLAB_SMALL:

        shift = slab & D_SLAB_SHIFT_MASK;
        size = (usz)1 << shift;

        if ((usz)p & (size - 1)) {
            goto wrong_chunk;
        }

        n = ((usz)p & (G_PageSize - 1)) >> shift;
        m = (usz)1 << (n % (8 * sizeof(usz)));
        n /= 8 * sizeof(usz);
        bitmap = (usz*)
            ((usz)p & ~((usz)G_PageSize - 1));

        if (bitmap[n] & m) {
            slot = shift - mMinShift;

            if (page->mNext == NULL) {
                slots = getSlots();

                page->mNext = slots[slot].mNext;
                slots[slot].mNext = page;

                page->mPrev = (usz)&slots[slot] | D_SLAB_SMALL;
                page->mNext->mPrev = (usz)page | D_SLAB_SMALL;
            }

            bitmap[n] &= ~m;

            n = (G_PageSize >> shift) / ((1 << shift) * 8);

            if (n == 0) {
                n = 1;
            }

            i = n / (8 * sizeof(usz));
            m = ((usz)1 << (n % (8 * sizeof(usz)))) - 1;

            if (bitmap[i] & ~m) {
                goto done;
            }

            map = (G_PageSize >> shift) / (8 * sizeof(usz));

            for (i = i + 1; i < map; i++) {
                if (bitmap[i]) {
                    goto done;
                }
            }

            freePages(page, 1);

            mStats[slot].mTotal -= (G_PageSize >> shift) - n;

            goto done;
        }

        goto chunk_already_free;

    case D_SLAB_EXACT:

        m = (usz)1 <<
            (((usz)p & (G_PageSize - 1)) >> G_ExactSlabSizeShift);
        size = G_ExactSlabSize;

        if ((usz)p & (size - 1)) {
            goto wrong_chunk;
        }

        if (slab & m) {
            slot = G_ExactSlabSizeShift - mMinShift;

            if (slab == D_SLAB_BUSY) {
                slots = getSlots();

                page->mNext = slots[slot].mNext;
                slots[slot].mNext = page;

                page->mPrev = (usz)&slots[slot] | D_SLAB_EXACT;
                page->mNext->mPrev = (usz)page | D_SLAB_EXACT;
            }

            page->mSlab &= ~m;

            if (page->mSlab) {
                goto done;
            }

            freePages(page, 1);

            mStats[slot].mTotal -= 8 * sizeof(usz);

            goto done;
        }

        goto chunk_already_free;

    case D_SLAB_BIG:

        shift = slab & D_SLAB_SHIFT_MASK;
        size = (usz)1 << shift;

        if ((usz)p & (size - 1)) {
            goto wrong_chunk;
        }

        m = (usz)1 << ((((usz)p & (G_PageSize - 1)) >> shift)
            + D_SLAB_MAP_SHIFT);

        if (slab & m) {
            slot = shift - mMinShift;

            if (page->mNext == NULL) {
                slots = getSlots();

                page->mNext = slots[slot].mNext;
                slots[slot].mNext = page;

                page->mPrev = (usz)&slots[slot] | D_SLAB_BIG;
                page->mNext->mPrev = (usz)page | D_SLAB_BIG;
            }

            page->mSlab &= ~m;

            if (page->mSlab & D_SLAB_MAP_MASK) {
                goto done;
            }

            freePages(page, 1);

            mStats[slot].mTotal -= G_PageSize >> shift;

            goto done;
        }

        goto chunk_already_free;

    case D_SLAB_PAGE:

        if ((usz)p & (G_PageSize - 1)) {
            goto wrong_chunk;
        }

        if (!(slab & D_SLAB_PAGE_START)) {
            Logger::logError("freeMem(): page is already free");
            goto fail;
        }

        if (slab == D_SLAB_PAGE_BUSY) {
            Logger::logError("freeMem(): pointer to wrong page");
            goto fail;
        }

        size = slab & ~D_SLAB_PAGE_START;

        freePages(page, size);

        DSLAB_JUNK(p, size << G_PageSizeShift);

        return;
    }

    /* not reached */

    return;


    done:

    mStats[slot].mUsed--;

    DSLAB_JUNK(p, size);

    return;

    wrong_chunk:

    Logger::logError("freeMem(): pointer to wrong chunk");

    goto fail;

    chunk_already_free:

    Logger::logError("freeMem(): chunk is already free");

    fail:
    return;
}


MemPage* MemSlabPool::allocPages(usz pages) {
    MemPage* page, * p;

    for (page = mFreePage.mNext; page != &mFreePage; page = page->mNext) {

        if (page->mSlab >= pages) {

            if (page->mSlab > pages) {
                page[page->mSlab - 1].mPrev = (usz)&page[pages];

                page[pages].mSlab = page->mSlab - pages;
                page[pages].mNext = page->mNext;
                page[pages].mPrev = page->mPrev;

                p = (MemPage*)page->mPrev;
                p->mNext = &page[pages];
                page->mNext->mPrev = (usz)&page[pages];

            } else {
                p = (MemPage*)page->mPrev;
                p->mNext = page->mNext;
                page->mNext->mPrev = page->mPrev;
            }

            page->mSlab = pages | D_SLAB_PAGE_START;
            page->mNext = NULL;
            page->mPrev = D_SLAB_PAGE;

            mFreePageCount -= pages;

            if (--pages == 0) {
                return page;
            }

            for (p = page + 1; pages; pages--) {
                p->mSlab = D_SLAB_PAGE_BUSY;
                p->mNext = NULL;
                p->mPrev = D_SLAB_PAGE;
                p++;
            }

            return page;
        }
    }

    if (this->mLogNoMem) {
        Logger::logError("allocMem() failed: no memory");
    }

    return NULL;
}


void MemSlabPool::freePages(MemPage* page, usz pages) {
    MemPage* prev, * join;

    mFreePageCount += pages;

    page->mSlab = pages--;

    if (pages) {
        memset(&page[1], 0, pages * sizeof(MemPage));
    }

    if (page->mNext) {
        prev = DGET_PAGE_PREV(page);
        prev->mNext = page->mNext;
        page->mNext->mPrev = page->mPrev;
    }

    join = page + page->mSlab;

    if (join < mLastPage) {

        if (DGET_PAGE_TYPE(join) == D_SLAB_PAGE) {

            if (join->mNext != NULL) {
                pages += join->mSlab;
                page->mSlab += join->mSlab;

                prev = DGET_PAGE_PREV(join);
                prev->mNext = join->mNext;
                join->mNext->mPrev = join->mPrev;

                join->mSlab = D_SLAB_PAGE_FREE;
                join->mNext = NULL;
                join->mPrev = D_SLAB_PAGE;
            }
        }
    }

    if (page > mPages) {
        join = page - 1;

        if (DGET_PAGE_TYPE(join) == D_SLAB_PAGE) {

            if (join->mSlab == D_SLAB_PAGE_FREE) {
                join = DGET_PAGE_PREV(join);
            }

            if (join->mNext != NULL) {
                pages += join->mSlab;
                join->mSlab += page->mSlab;

                prev = DGET_PAGE_PREV(join);
                prev->mNext = join->mNext;
                join->mNext->mPrev = join->mPrev;

                page->mSlab = D_SLAB_PAGE_FREE;
                page->mNext = NULL;
                page->mPrev = D_SLAB_PAGE;

                page = join;
            }
        }
    }

    if (pages) {
        page[pages].mPrev = (usz)page;
    }

    page->mPrev = (usz)&mFreePage;
    page->mNext = mFreePage.mNext;

    page->mNext->mPrev = (usz)page;

    mFreePage.mNext = page;
}



#ifdef DDEBUG
////////////////////////////////////////////
int AppTestMemSlab(int argc, char** argv) {
    printf("main>>start slab\n");

    usz  cmsz = 4096 * 16;
    char* cpool = new char[cmsz];
    char* cend = cpool + cmsz;
    app::MemSlabPool* mpool = new (cpool) app::MemSlabPool(cmsz);
    void* mid = mpool->allocMem(128);
    mpool->freeMem(mid);
    mid = mpool->allocMem(5);
    mpool->freeMem(mid);
    mid = mpool->allocMem(3000);
    mpool->freeMem(mid);
    delete[] cpool;
    printf("main>>stop slab\n");
    printf("main>>exit\n");
    return 0;
}

#endif

}//namespace app



