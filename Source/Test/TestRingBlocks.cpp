#include "RingBlocks.h"
#include <atomic>
#include <thread>
#include <string.h>
#include "Logger.h"
#include "Timer.h"

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
            DDLOG(ELL_DEBUG, "test_read: msg[%6d] = %s, [%4u,%4u]", cnt, buf->mBuf + buf->mOffset, buf->mUsed,
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

} // namespace app