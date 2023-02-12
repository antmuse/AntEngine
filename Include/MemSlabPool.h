#ifndef APP_MEMSLAB_H
#define APP_MEMSLAB_H

#include "Nocopy.h"
#include "Spinlock.h"

namespace app {

struct MemPage {
    usz mSlab;
    MemPage* mNext;
    usz mPrev;
};


struct MemStat {
    usz mTotal;
    usz mUsed;
    usz mRequests;
    usz mFails;
};

//原始内存首地址=this
class MemSlabPool : public Nocopy {
public:
    MemSlabPool() = delete;
    MemSlabPool(usz msz);
    ~MemSlabPool() { }

    void initSlabSize();

    u32 getStateCount();

    void* allocMem(usz size);
    void* allocMemNolock(usz size);
    void* callocMem(usz size);
    void* callocMemNolock(usz size);
    void freeMem(void* p);
    void freeMemNolock(void* p);

    usz mTotalSize;
    Spinlock mLock;    //TODO>>use RWLock?
    usz  mMinSize;     //最小8
    usz  mMinShift;    //最小3，mMinSize = 1<<mMinShift
    MemPage* mPages;
    MemPage* mLastPage;
    MemPage mFreePage;
    MemStat* mStats;
    usz mFreePageCount;   //count of free pages
    u8* mStartPos;        //页内存首地址
    u8* mEndPos;          //原始内存尾地址
    u32 mLogNoMem : 1;

private:
    void initSlab();
    MemPage* allocPages(usz pages);
    void freePages(MemPage* page, usz pages);
    usz getPageAddr(MemPage* page);
    MemPage* getSlots() {
        return (MemPage*)((u8*)(this) + sizeof(MemSlabPool));
    }
};




}//namespace app

#endif // APP_MEMSLAB_H
