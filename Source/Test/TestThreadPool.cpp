#include <iostream>
#include <vector>
#include <chrono>
#include "ThreadPool.h"
#include "MemoryHub.h"

namespace app {

static MemoryHub* GHUB = nullptr;
static std::atomic<s32> GHUB_CNT[MemoryHub::EMT_DEFAULT + 1];

static void AppTestMemHub() {
    usz msz = (rand() % 13248ULL) + 32;
    s8* mmm = GHUB->allocateAndClear(msz);
    snprintf(mmm, 32, "msz=%llu", msz);
    if (msz > 10240) {
        ++GHUB_CNT[MemoryHub::EMT_DEFAULT];
    } else if (msz > 8192) {
        ++GHUB_CNT[MemoryHub::EMT_10K];
    } else if (msz > 4096) {
        ++GHUB_CNT[MemoryHub::EMT_8192];
    } else if (msz > 2048) {
        ++GHUB_CNT[MemoryHub::EMT_4096];
    } else if (msz > 1024) {
        ++GHUB_CNT[MemoryHub::EMT_2048];
    } else if (msz > 512) {
        ++GHUB_CNT[MemoryHub::EMT_1024];
    } else if (msz > 256) {
        ++GHUB_CNT[MemoryHub::EMT_512];
    } else if (msz > 128) {
        ++GHUB_CNT[MemoryHub::EMT_256];
    } else {
        ++GHUB_CNT[MemoryHub::EMT_128];
    }
    GHUB->release(mmm);
}


static void AppMyTask(std::atomic<s64>* val) {
    ++(*val);
    AppTestMemHub();
}


class MyTask {
public:
    MyTask() :mID(0) {
        printf("MyTask=%p\n", this);
    }
    virtual ~MyTask() {
    }
    virtual void show1() {
        printf("MyTask2=%p\n", this);
    }
    virtual void run(std::atomic<s64>* val) {
        ++(*val);
        //printf("MyTask::run=%p,val=%p\n", this, val);
        AppTestMemHub();
    }
private:
    s32 mID;
};


class MyTask2 :public MyTask {
public:
    MyTask2() :mID2(0) {
        printf("MyTask2=%p\n", this);
    }
    virtual void show2() {
        printf("MyTask2=%p\n", this);
    }
    virtual void run(std::atomic<s64>* val) {
        ++(*val);
        ++(*val);
        //printf("MyTask2::run=%p,val=%p\n", this, val);
        AppTestMemHub();
    }
private:
    s32 mID2;
};


s32 AppTestThreadPool(s32 argc, s8** argv) {
    for (s32 i = sizeof(GHUB_CNT) / sizeof(GHUB_CNT[0]) - 1; i >= 0; --i) {
        GHUB_CNT[i].store(0);
    }
    GHUB = new MemoryHub();
    ThreadPool pool;
    s32 posted = 30;
    pool.start(30);
    printf("AppTestThreadPool>>pool thread=%d\n", posted);

    std::atomic<s64> cnt(0);
    std::atomic<s64> urg(0);
    MyTask tsk;
    MyTask2 tsk2;
    bool urgent = false;
    std::chrono::milliseconds gap(10);

    //------------------step1
    pool.setMaxAllocated(100);
    for (posted = 0; posted < 50000; ++posted) {
        urgent = (0x8 & rand()) > 0;
        pool.postTask(AppMyTask, urgent ? &urg : &cnt, urgent);
    }
    while (pool.getTaskCount() > 0) {
        std::this_thread::sleep_for(gap);
    }
    printf("step1>>pool Allocated=%d, cnt = %lld, urg=%lld, total=%lld/%d\n\n",
        pool.getAllocated(), cnt.load(), urg.load(), cnt.load() + urg.load(), posted);
    cnt.exchange(0);
    urg.exchange(0);


    //------------------step2
    pool.setMaxAllocated(200);
    for (posted = 0; posted < 80000; ++posted) {
        urgent = (0x4 & rand()) > 0;
        pool.postTask(&MyTask::run, &tsk, urgent ? &urg : &cnt, urgent);
    }
    while (pool.getTaskCount() > 0) {
        std::this_thread::sleep_for(gap);
    }
    printf("step3>>pool Allocated=%d, cnt = %lld, urg=%lld, total=%lld/%d\n\n",
        pool.getAllocated(), cnt.load(), urg.load(), cnt.load() + urg.load(), posted);
    cnt.exchange(0);
    urg.exchange(0);


    //------------------step3
    pool.setMaxAllocated(10000);
    for (posted = 0; posted < 70000; ++posted) {
        urgent = (0x10 & rand()) > 0;
        pool.postTask(&MyTask2::run, &tsk2, urgent ? &urg : &cnt, urgent);
    }
    pool.stop();
    printf("step3>>pool Allocated=%d, cnt = %lld, urg=%lld, total=%lld/%d\n\n",
        pool.getAllocated(), cnt.load(), urg.load(), cnt.load() + urg.load(), posted);


    posted = 0;
    for (s32 i = sizeof(GHUB_CNT) / sizeof(GHUB_CNT[0]) - 1; i >= 0; --i) {
        posted += GHUB_CNT[i].load();
        printf("step4>>mem pool[%d] = %d\n", i, GHUB_CNT[i].load());
    }
    printf("step4>>mem pool total = %d\n\n", posted);
    GHUB->drop();
    return 0;
}


}//namespace app