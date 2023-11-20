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


#ifndef APP_IOURING_H
#define APP_IOURING_H

#include "Nocopy.h"
#include "Linux/Request.h"

namespace app {

enum IORingSetupFlag {
    IORING_SETUP_SQPOLL = 2u, // SQPOLL mode, make the kernal create SQ thread, and add io_uring FD in epoll
};

enum {
    IORING_FEAT_SINGLE_MMAP = 1u,
    IORING_FEAT_NODROP = 2u,
    IORING_FEAT_RSRC_TAGS = 1024u, /* linux v5.13 */
};

enum {
    IORING_MKDIRAT_SYMLINKAT_LINKAT = 1u,
};


enum {
    IORING_ENTER_GETEVENTS = 1u,
    IORING_ENTER_SQ_WAKEUP = 2u,
};


enum {
    IORING_SQ_NEED_WAKEUP = 1u,
    IORING_SQ_CQ_OVERFLOW = 2u,
};


/**
 * @beief must keep in order
 */
enum IORingOP {
    IORING_OP_NOP = 0,
    IORING_OP_READV = 1,  // preadv()
    IORING_OP_WRITEV = 2, // pwritev()
    IORING_OP_FSYNC = 3,
    IORING_OP_READ_FIXED,
    IORING_OP_WRITE_FIXED,
    IORING_OP_POLL_ADD,
    IORING_OP_POLL_REMOVE,
    IORING_OP_SYNC_FILE_RANGE,
    IORING_OP_SENDMSG,
    IORING_OP_RECVMSG,
    IORING_OP_TIMEOUT,
    IORING_OP_TIMEOUT_REMOVE,
    IORING_OP_ACCEPT,
    IORING_OP_ASYNC_CANCEL,
    IORING_OP_LINK_TIMEOUT,
    IORING_OP_CONNECT,
    IORING_OP_FALLOCATE,
    IORING_OP_OPENAT = 18,
    IORING_OP_CLOSE = 19,
    IORING_OP_FILES_UPDATE,
    IORING_OP_STATX = 21,
    IORING_OP_READ = 22,  // pread()
    IORING_OP_WRITE = 23, // pwrite()
    IORING_OP_FADVISE,
    IORING_OP_MADVISE,
    IORING_OP_SEND = 26,
    IORING_OP_RECV = 27,
    IORING_OP_OPENAT2,
    IORING_OP_EPOLL_CTL = 29,
    IORING_OP_SPLICE,
    IORING_OP_PROVIDE_BUFFERS,
    IORING_OP_REMOVE_BUFFERS,
    IORING_OP_TEE,
    IORING_OP_SHUTDOWN,
    IORING_OP_RENAMEAT = 35,
    IORING_OP_UNLINKAT = 36,
    IORING_OP_MKDIRAT = 37,
    IORING_OP_SYMLINKAT = 38,
    IORING_OP_LINKAT = 39,
    IORING_OP_MSG_RING,
    IORING_OP_FSETXATTR,
    IORING_OP_SETXATTR,
    IORING_OP_FGETXATTR,
    IORING_OP_GETXATTR,
    IORING_OP_SOCKET,
    IORING_OP_URING_CMD,
    IORING_OP_SEND_ZC,
    IORING_OP_SENDMSG_ZC,
    IORING_OP_READ_MULTISHOT,
    IORING_OP_WAITID,
    IORING_OP_FUTEX_WAIT,
    IORING_OP_FUTEX_WAKE,
    IORING_OP_FUTEX_WAITV,

    // this goes last, obviously.
    IORING_OP_MAX
};

class IOURing final : public Nocopy {
public:
    IOURing();

    ~IOURing();

    /**
     * @brief open a io_uring and add it in epoll if (\p flags & IORING_SETUP_SQPOLL).
     *
     * @param epollfd the epoll FD
     * @param entries max SQE of uring, will be up to power of 2.
     * @param flags ctrl bits, @see IORingSetupFlag
     * @return true if success, else failed.
     */
    bool open(s32 epollfd, u32 entries, u32 flags);

    void close();

    void updatePending();

    s32 postReq(RequestFD* req);

    s32 getRingFD() const {
        return mRingFD;
    }

    u32 getFlyRequest() const {
        return mFlyRequest;
    }

    u32 getSize() const {
        return mFlyRequest + mWaitPostSize;
    }

private:
    u32* mHeadSQ;
    u32* mTailSQ;
    u32* mArraySQ;
    u32* mFlagsSQ;
    u32 mMaskSQ;
    u32 mMaskCQ;
    u32* mHeadCQ;
    u32* mTailCQ;
    void* mSQ;  // pointer to munmap() on event loop teardown
    void* mCQE; // pointer to array of struct URingCQE
    void* mSQE; // pointer to array of struct URingSQE
    usz mLenSQ;
    usz mLenCQ;
    usz mLenMax;
    usz mLenSQE;
    s32 mRingFD;
    u32 mFlyRequest;
    u32 mFlags;
    u32 mWaitPostSize;         // count of RequestFDs in \p mWaitPostQueue
    RequestFD* mWaitPostQueue; // point to tail,mWaitPostQueue->mNext is head

    void* getSQE();
    void wakeupThreadSQ();
    void postQueue();
};

} // namespace app



#endif // APP_IOURING_H
