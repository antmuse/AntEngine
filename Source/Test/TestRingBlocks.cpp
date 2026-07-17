#include <atomic>
#include <thread>
#include <string.h>
#include "Logger.h"
#include "Timer.h"
#include "TVector.h"
#include "RingBlocks.h"
#include "RingBufferFixed.h"
#include "Engine.h"
#include "MapFile.h"

namespace app {

const int GMAX_MSG = 64;

void test_write(RingBlocks* rin) {
    std::chrono::milliseconds waitms(10);
    int cnt = 0;
    const usz blocksz = rin->getBlockSize();
    usz snp;
    RingBlocks::Block* buf;
    for (; cnt < GMAX_MSG;) {
        buf = rin->getTailBlock();
        if (buf) {
            ++cnt;
            buf->mOffset = 0;
            snp = snprintf(buf->mBuf, blocksz, "test_write:%d", cnt);
            buf->mUsed = static_cast<u16>(snp > blocksz ? blocksz : snp);
            rin->commitTail();
        } else {
            printf("%lld, test_write: sleep total = %d\n", Timer::getRealTime(), cnt);
            std::this_thread::sleep_for(waitms);
        }
    }
    printf("test_write: total = %d\n", cnt);
}

void test_read(RingBlocks* rin) {
    std::chrono::milliseconds waitms(10);
    int cnt = 0;
    RingBlocks::Block* buf;
    for (; cnt < GMAX_MSG;) {
        buf = rin->getHeadBlock();
        if (buf) {
            ++cnt;
            DLOG(ELL_DEBUG, "test_read: msg[%6d] = %s, [%4u,%4u]", cnt, buf->mBuf + buf->mOffset, buf->mUsed,
                buf->mOffset);
            buf->mUsed = 0;
            buf->mOffset = 0;
            rin->commitHead();
        } else {
            printf("%lld, test_read: sleep total = %d\n", Timer::getRealTime(), cnt);
            std::this_thread::sleep_for(waitms);
        }
    }
    printf("test_read: total = %d\n", cnt);
}

s32 AppTestRingBlocks(s32 argc, s8** argv) {
    RingBlocks rin;
    rin.open(31, 811);
    std::thread wer(test_write, &rin);
    std::thread rer(test_read, &rin);
    printf("test_thread run...\n");

    rer.join();
    wer.join();

    RingBlocks::Block* buf = rin.getHeadBlock();
    if (buf) {
        printf("test_rin fail\n");
    }
    usz msgsz = 64 * 1024;
    s8* msg = new s8[msgsz];
    snprintf(msg, msgsz, "streamsg.streamsg.streamsg.streamsg;;;\n");
    usz sized = rin.write(msg, msgsz);
    printf("test_rin stream w=%llu/%llu\n", sized, msgsz);
    buf = rin.getHeadBlock();
    sized = rin.read(msg, msgsz);
    printf("test_rin stream r=%llu/%llu\n", sized, msgsz);
    buf = rin.getHeadBlock();
    delete[] msg;
    rin.close();

    return 0;
}



class RingbufTest {
public:
    RingbufTest(const s8* wtxt) : mWriteTxt(wtxt) {
    }
    void run(void* param) {
        usz memlen = 1024;
        if (!mMapfile.createMem(memlen, mMemName, false, true, true)) {
            DLOG(ELL_ERROR, "createMem fail = %s", mMemName);
            return;
        }
        bool writer = 1 == ((usz)param);
        Engine& eng = Engine::getInstance();
        void* allmem = mMapfile.getMem();
        buff = reinterpret_cast<RingBufferFixed*>(allmem);
        if (mMapfile.isCreator()) {
            DLOG(ELL_INFO, "RingBufferFixed init start = %s, %p", mMemName, allmem);
            if (!buff->init(allmem, memlen)) {
                DLOG(ELL_INFO, "RingBufferFixed init fail = %s, %p", mMemName, allmem);
                return;
            }
            DLOG(ELL_INFO, "RingBufferFixed init success = %s, %p", mMemName, allmem);
        } else {
            DLOG(ELL_INFO, "RingBufferFixed wait init = %, %p", mMemName, allmem);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        s8 buf[128];
        usz len;
        ssz retlen = 0;
        usz cnt = 0;
        if (writer) {
            DLOG(ELL_INFO, "AppRingBufferFixed, writer start");
            for (usz i = 0; retlen >= 0 && EPS_RUNNING == eng.getStatus(); ++i) {
                len = 1 + snprintf(buf, sizeof(buf), "%s/%08llu", mWriteTxt.data(), i);
                buff->getSpinlock().lock();
                retlen = buff->write(buf, len);
                buff->getSpinlock().unlock();
                if (0 == retlen) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                } else if (retlen > 0) {
                    ++cnt;
                    buff->getFutex().tryUnlock();
                }
            }
            DLOG(ELL_INFO, "AppRingBufferFixed, writer cnt = %llu", cnt);
        } else {
            DLOG(ELL_INFO, "AppRingBufferFixed, reader start");
            for (; retlen >= 0 && EPS_RUNNING == eng.getStatus();) {
                buff->getSpinlock().lock();
                retlen = buff->read(buf, sizeof(buf));
                buff->getSpinlock().unlock();
                if (0 == retlen) {
                    buff->getFutex().lock();
                    // std::this_thread::sleep_for(std::chrono::microseconds(10));
                } else if (retlen > 0) {
                    ++cnt;
                    printf("[%llu]: %s\n", cnt, buf);
                }
            }
            DLOG(ELL_INFO, "AppRingBufferFixed, reader cnt = %llu", cnt);
        }
        mMapfile.closeAll();
        delete this;
    }

private:
    RingBufferFixed* buff = nullptr;
    const s8* mMemName = "GMEM/RingbufTest.mem";
    MapFile mMapfile;
    String mWriteTxt;
};


s32 AppRingBufferFixed(s32 argc, s8** argv) {
    bool writer = (argc > 2 && argv[2][0] == '2') ? true : false;
    RingbufTest* task = new RingbufTest((argc > 3 ? argv[3] : "AppRingBufferFixed"));
    Engine::getInstance().getThreadPool().postTask(&RingbufTest::run, task, (void*)writer);
    return 0;
}

} // namespace app