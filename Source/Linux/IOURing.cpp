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


#include "Linux/IOURing.h"
#include "System.h"
#include "Net/Socket.h"
#include "Logger.h"
#include "Loop.h"
#include "HandleFile.h"

#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <atomic>

#ifndef __NR_io_uring_setup
#define __NR_io_uring_setup 425
#endif

#ifndef __NR_io_uring_enter
#define __NR_io_uring_enter 426
#endif

#ifndef __NR_io_uring_register
#define __NR_io_uring_register 427
#endif

namespace app {

// Magic offsets for the application to mmap the data it needs
const usz IORING_OFF_SQ_RING = 0;
const usz IORING_OFF_CQ_RING = 0x8000000ULL;
const usz IORING_OFF_SQES = 0x10000000ULL;
const usz IORING_OFF_PBUF_RING = 0x80000000ULL;
const usz IORING_OFF_PBUF_SHIFT = 16;
const usz IORING_OFF_MMAP_MASK = 0xf8000000ULL;


const u32 G_MAX_WAIT_REQ = 2000;

struct CQRingOffsets {
    u32 head;
    u32 tail;
    u32 ring_mask;
    u32 ring_entries;
    u32 overflow;
    u32 cqes;
    u64 reserved0;
    u64 reserved1;
};

struct SQRingOffsets {
    u32 head;
    u32 tail;
    u32 ring_mask;
    u32 ring_entries;
    u32 flags;
    u32 dropped;
    u32 array;
    u32 reserved0;
    u64 reserved1;
};

static_assert(40 == sizeof(CQRingOffsets));
static_assert(40 == sizeof(SQRingOffsets));

struct URingCQE {
    u64 user_data;
    s32 res; // sucess bytes if >=0, else err code.
    u32 flags;
};

static_assert(16 == sizeof(URingCQE));

struct URingSQE {
    u8 opcode;
    u8 flags;
    u16 ioprio;
    s32 fd;
    union {
        u64 off; // offset of file
        u64 addr2;
    };
    union {
        u64 addr;
    };
    u32 len;
    union {
        u32 rw_flags;
        u32 fsync_flags;
        u32 open_flags;
        u32 statx_flags;
    };
    u64 user_data;
    union {
        u16 buf_index;
        u64 pad[3];
    };
};


static_assert(64 == sizeof(URingSQE));
static_assert(0 == offsetof(URingSQE, opcode));
static_assert(1 == offsetof(URingSQE, flags));
static_assert(2 == offsetof(URingSQE, ioprio));
static_assert(4 == offsetof(URingSQE, fd));
static_assert(8 == offsetof(URingSQE, off));
static_assert(16 == offsetof(URingSQE, addr));
static_assert(24 == offsetof(URingSQE, len));
static_assert(28 == offsetof(URingSQE, rw_flags));
static_assert(32 == offsetof(URingSQE, user_data));
static_assert(40 == offsetof(URingSQE, buf_index));


struct URingParam {
    u32 sq_entries;
    u32 cq_entries;
    u32 flags;
    u32 sq_thread_cpu;
    u32 sq_thread_idle;
    u32 features;
    u32 reserved[4];
    SQRingOffsets sq_off; // 40 bytes
    CQRingOffsets cq_off; // 40 bytes
};

static_assert(40 + 40 + 40 == sizeof(URingParam));
static_assert(40 == offsetof(URingParam, sq_off));
static_assert(80 == offsetof(URingParam, cq_off));


static s32 AppIOURing_setup(s32 entries, URingParam* params) {
    return syscall(__NR_io_uring_setup, entries, params);
}

static s32 AppIOURing_enter(s32 fd, u32 to_submit, u32 min_complete, u32 flags) {
    /* io_uring_enter used to take a sigset_t but it's unused
     * in newer kernels unless IORING_ENTER_EXT_ARG is set,
     * in which case it takes a struct io_uring_getevents_arg.
     */
    return syscall(__NR_io_uring_enter, fd, to_submit, min_complete, flags, NULL, 0L);
}

static s32 AppIOURing_register(s32 fd, u32 opcode, void* arg, u32 nargs) {
    return syscall(__NR_io_uring_register, fd, opcode, arg, nargs);
}


IOURing::IOURing() {
    memset(this, 0, sizeof(*this));
    mRingFD = -1;
}


IOURing::~IOURing() {
    close();
}

bool IOURing::open(s32 epollfd, u32 entries, u32 flags) {
    URingParam params;
    struct epoll_event evt;
    size_t cqlen;
    size_t sqlen;
    size_t maxlen;
    size_t sqelen;
    u32 i;
    s8* sq = (s8*)MAP_FAILED;
    s8* sqe = (s8*)MAP_FAILED;

    /* SQPOLL required CAP_SYS_NICE until linux v5.12 relaxed that requirement.
     * Mostly academic because we check for a v5.13 kernel afterwards anyway.
     */
    memset(&params, 0, sizeof(params));
    params.flags = flags;

    if (flags & IORING_SETUP_SQPOLL)
        params.sq_thread_idle = 10; /* milliseconds */

    /* Kernel returns a file descriptor with O_CLOEXEC flag set. */
    s32 ringfd = AppIOURing_setup(entries, &params);
    if (ringfd == -1) {
        return false;
    }
    /* IORING_FEAT_RSRC_TAGS is used to detect linux v5.13 but what we're
     * actually detecting is whether IORING_OP_STATX works with SQPOLL.
     */
    if (!(params.features & IORING_FEAT_RSRC_TAGS))
        goto GT_INIT_FAIL;

    /* Implied by IORING_FEAT_RSRC_TAGS but checked explicitly anyway. */
    if (!(params.features & IORING_FEAT_SINGLE_MMAP))
        goto GT_INIT_FAIL;

    /* Implied by IORING_FEAT_RSRC_TAGS but checked explicitly anyway. */
    if (!(params.features & IORING_FEAT_NODROP))
        goto GT_INIT_FAIL;

    sqlen = params.sq_off.array + params.sq_entries * sizeof(u32);
    cqlen = params.cq_off.cqes + params.cq_entries * sizeof(URingCQE);
    maxlen = sqlen < cqlen ? cqlen : sqlen;
    sqelen = params.sq_entries * sizeof(URingSQE);

    sq = (s8*)mmap(0, maxlen, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ringfd, IORING_OFF_SQ_RING);

    sqe = (s8*)mmap(0, sqelen, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, ringfd, IORING_OFF_SQES);

    if (sq == MAP_FAILED || sqe == MAP_FAILED) {
        goto GT_INIT_FAIL;
    }
    if (flags & IORING_SETUP_SQPOLL) {
        /* Only interested in completion events. To get notified when
         * the kernel pulls items from the submission ring, add POLLOUT.
         */
        memset(&evt, 0, sizeof(evt));
        evt.events = EPOLLIN;
        evt.data.ptr = this;

        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ringfd, &evt)) {
            goto GT_INIT_FAIL;
        }
    }

    mHeadSQ = (u32*)(sq + params.sq_off.head);
    mTailSQ = (u32*)(sq + params.sq_off.tail);
    mMaskSQ = *(u32*)(sq + params.sq_off.ring_mask);
    mArraySQ = (u32*)(sq + params.sq_off.array);
    mFlagsSQ = (u32*)(sq + params.sq_off.flags);
    mHeadCQ = (u32*)(sq + params.cq_off.head);
    mTailCQ = (u32*)(sq + params.cq_off.tail);
    mMaskCQ = *(u32*)(sq + params.cq_off.ring_mask);
    mSQ = sq;
    mCQE = sq + params.cq_off.cqes;
    mSQE = sqe;
    mLenSQ = sqlen;
    mLenCQ = cqlen;
    mLenMax = maxlen;
    mLenSQE = sqelen;
    mRingFD = ringfd;
    mFlyRequest = 0;
    mFlags = 0;
    mWaitPostSize = 0;
    mWaitPostQueue = nullptr;


    // need (kernel_version >= 5.15.0)
    mFlags |= IORING_MKDIRAT_SYMLINKAT_LINKAT;

    for (i = 0; i <= mMaskSQ; i++) {
        mArraySQ[i] = i; // Slot -> sqe identity mapping
    }
    return true;

GT_INIT_FAIL:
    if (sq != MAP_FAILED) {
        munmap(sq, maxlen);
    }
    if (sqe != MAP_FAILED) {
        munmap(sqe, sqelen);
    }
    ::close(ringfd);
    return false;
}


void IOURing::close() {
    if (-1 != mRingFD) {
        munmap(mSQ, mLenMax);
        munmap(mSQE, mLenSQE);
        ::close(mRingFD);
        // memset(this, 0, sizeof(*this));
        mRingFD = -1;
    }
}


void IOURing::wakeupThreadSQ() {
    /* std::atomic_store_explicit(reinterpret_cast<std::atomic<u32>*>(mTailSQ), *mTailSQ + 1,
     * std::memory_order_release);*/

    u32 flags = std::atomic_load_explicit(reinterpret_cast<std::atomic<u32>*>(mFlagsSQ), std::memory_order_acquire);
    if (flags & IORING_SQ_NEED_WAKEUP) {
        if (AppIOURing_enter(mRingFD, 0, 0, IORING_ENTER_SQ_WAKEUP)) {
            if (errno != EOWNERDEAD) {                               // Kernel bug. Harmless, ignore
                Logger::log(ELL_CRITICAL, "io_uring_enter(wakeup)"); // Can't happen
            }
        }
    }
}


void* IOURing::getSQE() {
    if (mRingFD == -1) {
        return nullptr;
    }

    u32 head = std::atomic_load_explicit(reinterpret_cast<std::atomic<u32>*>(mHeadSQ), std::memory_order_acquire);
    u32 tail = *mTailSQ;
    u32 mask = mMaskSQ;

    if ((head & mask) == ((tail + 1) & mask)) {
        return nullptr; // No room in ring buffer. TODO: maybe flush it?
    }
    u32 slot = tail & mask;
    URingSQE* sqe = reinterpret_cast<URingSQE*>(mSQE);
    sqe = &sqe[slot];
    memset(sqe, 0, sizeof(*sqe));
    return sqe;
}


void IOURing::postQueue() {
    if (!mWaitPostQueue) {
        return;
    }
    HandleFile* handle;
    URingSQE* sqe;
    RequestFD* req;
    // u32 cnt = 0;
    while (mWaitPostQueue) {
        sqe = reinterpret_cast<URingSQE*>(getSQE());
        if (!sqe) {
            break;
        }
        req = AppPopRingQueueHead_1(mWaitPostQueue);
        handle = reinterpret_cast<HandleFile*>(req->mHandle);
        --mWaitPostSize;
        mFlyRequest++;
        //++cnt;
        sqe->user_data = (u64)req;
        sqe->addr = (u64)req->mData;
        sqe->fd = handle->getHandle();
        sqe->len = req->mAllocated;
        sqe->off = req->mOffset; // default set offset = -1
        sqe->opcode = (ERT_READ == req->mType ? IORING_OP_READ : IORING_OP_WRITE);
        std::atomic_store_explicit(
            reinterpret_cast<std::atomic<u32>*>(mTailSQ), *mTailSQ + 1, std::memory_order_release);
    }
    wakeupThreadSQ();
}


s32 IOURing::postReq(RequestFD* req) {
    if (mWaitPostSize >= G_MAX_WAIT_REQ) {
        return EE_RETRY;
    }
    HandleFile* handle = reinterpret_cast<HandleFile*>(req->mHandle);
    handle->getLoop()->bindFly(handle);
    AppPushRingQueueTail_1(mWaitPostQueue, req);
    ++mWaitPostSize;
    postQueue();
    return EE_OK;
}


void IOURing::updatePending() {
    u32 head = *mHeadCQ;
    u32 tail = std::atomic_load_explicit(reinterpret_cast<std::atomic<u32>*>(mTailCQ), std::memory_order_acquire);
    u32 mask = mMaskCQ;
    URingCQE* cqe = reinterpret_cast<URingCQE*>(mCQE);
    URingCQE* nd;
    RequestFD* req;
    for (u32 i = head; i != tail; i++) {
        nd = &cqe[i & mask];

        req = (RequestFD*)(u64)nd->user_data;
        DASSERT(req->mHandle && req->mHandle->getType() == EHT_FILE);

        mFlyRequest--;

        // io_uring stores error codes as negative numbers
        if (nd->res >= 0) {
            req->mUsed += nd->res;
        } else {
            req->mError = nd->res;
        }
        req->mCall(req);
        HandleFile* handle = reinterpret_cast<HandleFile*>(req->mHandle);
        handle->getLoop()->unbindFly(handle);
    }

    std::atomic_store_explicit(reinterpret_cast<std::atomic<u32>*>(mHeadCQ), tail, std::memory_order_release);

    /* Check whether CQE's overflowed, if so enter the kernel to make them
     * available. Don't grab them immediately but in the next loop iteration to
     * avoid loop starvation. */
    u32 flags = std::atomic_load_explicit(reinterpret_cast<std::atomic<u32>*>(mFlagsSQ), std::memory_order_acquire);
    s32 rc;
    if (flags & IORING_SQ_CQ_OVERFLOW) {
        do {
            rc = AppIOURing_enter(mRingFD, 0, 0, IORING_ENTER_GETEVENTS);
        } while (rc == -1 && errno == EINTR);
        if (rc < 0) {
            Logger::log(ELL_CRITICAL, "io_uring_enter(getevents)"); // Can't happen
        }
    }
    postQueue();
}


} // namespace app
