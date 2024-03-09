#include <iostream>
#include <vector>
#include <chrono>
#include "ThreadPool.h"
#include "MemoryHub.h"
#include "Queue.h"

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
    MyTask() : mID(0) {
        printf("MyTask=%p\n", this);
    }
    virtual ~MyTask() {
    }
    virtual void show1() {
        printf("MyTask2=%p\n", this);
    }
    void run(std::atomic<s64>* val) {
        ++(*val);
        // printf("MyTask::run=%p,val=%p\n", this, val);
        AppTestMemHub();
    }

private:
    s32 mID;
};


class MyTask2 : public MyTask {
public:
    MyTask2() : mID2(0) {
        printf("MyTask2=%p\n", this);
    }
    virtual void show2() {
        printf("MyTask2=%p\n", this);
    }
    void run(std::atomic<s64>* val) {
        ++(*val);
        ++(*val);
        // printf("MyTask2::run=%p,val=%p\n", this, val);
        AppTestMemHub();
    }

private:
    s32 mID2;
};



// MyQueNode
class MyQueNode {
public:
    static std::atomic<s32> GID;
    static std::atomic<s32> G_CNT_R;
    static std::atomic<s32> G_CNT_R_FAIL;
    static std::atomic<s32> G_CNT_W;
    static s32 G_MAX_MSG;

    MyQueNode() : mID(++GID) {
    }

private:
    MyQueNode* mNext;
    std::atomic<s32> mID;
};
std::atomic<s32> MyQueNode::GID(0);
std::atomic<s32> MyQueNode::G_CNT_R(0);
std::atomic<s32> MyQueNode::G_CNT_R_FAIL(0);
std::atomic<s32> MyQueNode::G_CNT_W(0);
s32 MyQueNode::G_MAX_MSG = 200;

class QueueTask {
public:
    void read(Queue* que) {
        while (MyQueNode::G_CNT_R < MyQueNode::G_MAX_MSG) {
            MyQueNode* nd = (MyQueNode*)que->readMsg();
            if (nd) {
                ++MyQueNode::G_CNT_R;
                delete nd;
            } else {
                // printf("read=0\n");
                ++MyQueNode::G_CNT_R_FAIL;
            }
        }
    }
    void write(Queue* que) {
        while (true) {
            s32 cnt = ++MyQueNode::G_CNT_W;
            if (cnt > MyQueNode::G_MAX_MSG) {
                --MyQueNode::G_CNT_W;
                break;
            }
            MyQueNode* nd = new MyQueNode();
            que->writeMsg(nd);
        }
    }
};

s32 AppTestThreadPool(s32 argc, s8** argv) {
    const s32 threads = 30;
    for (s32 i = sizeof(GHUB_CNT) / sizeof(GHUB_CNT[0]) - 1; i >= 0; --i) {
        GHUB_CNT[i].store(0);
    }
    GHUB = new MemoryHub();
    ThreadPool pool;
    s32 posted = 30;
    pool.start(threads);
    printf("AppTestThreadPool>>pool thread=%d\n", threads);

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
    printf("step1>>pool Allocated=%d, cnt = %lld, urg=%lld, total=%lld/%d\n\n", pool.getAllocated(), cnt.load(),
        urg.load(), cnt.load() + urg.load(), posted);
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
    printf("step3>>pool Allocated=%d, cnt = %lld, urg=%lld, total=%lld/%d\n\n", pool.getAllocated(), cnt.load(),
        urg.load(), cnt.load() + urg.load(), posted);
    cnt.exchange(0);
    urg.exchange(0);


    //------------------step3
    pool.setMaxAllocated(10000);
    for (posted = 0; posted < 70000; ++posted) {
        urgent = (0x10 & rand()) > 0;
        pool.postTask(&MyTask2::run, &tsk2, urgent ? &urg : &cnt, urgent);
    }
    pool.stop();
    printf("step3>>pool Allocated=%d, cnt = %lld, urg=%lld, total=%lld/%d\n\n", pool.getAllocated(), cnt.load(),
        urg.load(), cnt.load() + urg.load(), posted);


    posted = 0;
    for (s32 i = sizeof(GHUB_CNT) / sizeof(GHUB_CNT[0]) - 1; i >= 0; --i) {
        posted += GHUB_CNT[i].load();
        printf("step4>>mem pool[%d] = %d\n", i, GHUB_CNT[i].load());
    }
    printf("step4>>mem pool total = %d\n\n", posted);
    GHUB->drop();



    // test queue
    MyQueNode::G_MAX_MSG = 5000000;
    printf("\n\nstep5>>test queue start, max = %d\n\n", MyQueNode::G_MAX_MSG);
    Queue* que = new Queue(0, 1000);
    QueueTask allwk[threads];
    pool.start(threads);
    que->setBlockRead(false);
    for (posted = 0; posted < threads / 2; ++posted) {
        pool.postTask(&QueueTask::write, allwk + posted, que);
    }
    std::this_thread::sleep_for(gap);
    printf("step5>>test queue, que.size = %llu\n\n", que->getMsgCount());
    for (; posted < threads; ++posted) {
        pool.postTask(&QueueTask::read, allwk + posted, que);
    }
    while (MyQueNode::G_CNT_W < MyQueNode::G_MAX_MSG) {
        std::this_thread::sleep_for(gap);
    }
    printf("step5>>test queue write done\n");
    while (MyQueNode::G_CNT_R < MyQueNode::G_MAX_MSG) {
        std::this_thread::sleep_for(gap);
    }

    f32 ratio = 100.f * MyQueNode::G_CNT_R_FAIL.load();
    ratio /= MyQueNode::G_MAX_MSG;
    printf("step5>>test queue stop, read0=%d=%.2f%%, que.size = %llu\n\n", MyQueNode::G_CNT_R_FAIL.load(), ratio,
        que->getMsgCount());
    que->setBlock(false);
    pool.stop();
    que->drop();


    return 0;
}

s32 AppTestMemPool(s32 argc, s8** argv) {
    const s32 psz = 1024 * 1024 * 8;
    MemPool* pool = MemPool::createMemPool(psz);
    pool->logPool();

    const s32 maxnd = 5000;
    s8** nd = new s8*[maxnd];
    for (s32 i = 0; i < maxnd; ++i) {
        s32 ndsz = rand() % 1000 + i + 8 * sizeof(s32);
#ifdef DDEBUG
        nd[i] = pool->allocate(ndsz, "test", __FILE__, __LINE__);
#else
        nd[i] = pool->allocate(ndsz);
#endif
        *(s32*)nd[i] = ndsz;
        snprintf(nd[i] + sizeof(s32), ndsz - sizeof(s32), ",%d", ndsz);
        printf("create[%p]=%d\n", nd[i], ndsz);
    }

    pool->logPool();

    for (s32 i = 0; i < maxnd; ++i) {
        s32 ndsz = *(s32*)nd[i];
        // printf("free[%p]=%d\n", nd[i], ndsz);
        pool->release(nd[i]);
    }

#ifdef DDEBUG
    nd[0] = pool->allocateAndClear(128, "last128", __FILE__, __LINE__);
    nd[1] = pool->allocateAndClear(32, "last32", __FILE__, __LINE__);
#else
    nd[0] = pool->allocateAndClear(128);
    nd[1] = pool->allocateAndClear(32);
#endif
    pool->release(nd[1]);
    pool->release(nd[0]);
    delete[] nd;
    pool->logPool();
    MemPool::releaseMemPool(pool);
    return 0;
}

} // namespace app