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


#include <stdio.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include "HandleFile.h"
#include "System.h"
#include "Engine.h"

namespace app {

static int io_setup(u32 nr_reqs, u32* ctx) {
    return syscall(SYS_io_setup, nr_reqs, ctx);
}

static int io_destroy(u32 ctx) {
    return syscall(SYS_io_destroy, ctx);
}

static int io_getevents(u32 ctx, long min_nr, long nr,
    struct io_event* events, struct timespec* tmo) {
    return syscall(SYS_io_getevents, ctx, min_nr, nr, events, tmo);
}

class HandleAIO {
public:
    HandleAIO() :mEventFD(-1) {
    }
    void init() {
        mEventFD = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        // SYS_gettid
                // if (posix_memalign(&buf, ALIGN_SIZE, RD_WR_SIZE))        {
                //     perror("posix_memalign");
                //     return 5;
                // }
    }


private:
    u32 mContext;
    s32 mEventFD;
};


HandleFile::HandleFile()
    :mFile(0)
    , mFileSize(0) {
    mType = EHT_FILE;
    mLoop = &Engine::getInstance().getLoop();

    //const usz aaa = sizeof(struct iocb);
}


HandleFile::~HandleFile() {
    close();
}


s32 HandleFile::close() {
    if (0 != mFile) {
        ::close(mFile);
        mFile = 0;
        mFilename.setLen(0);
        mFileSize = 0;
    }
    return EE_OK;
}

bool HandleFile::setFileSize(usz fsz) {
    //TODO>>
    return true;
}

s32 HandleFile::open(const String& fname, s32 flag) {
    close();
    mFilename = fname;

    s32 fmode = (flag & 1) > 0 ? 1 : 0;
    fmode |= (flag & 2) > 0 ? 2 : 0;

    s32 cmod = (flag & 4) > 0 ? 1 : 0;

    s32 attr = 0;

    //mFile = ::open(mFilename.c_str(), O_NONBLOCK | O_CREAT | O_RDWR | O_DIRECT, 0644);
    mFile = ::open(mFilename.c_str(), O_RDONLY, 0644);

    if (0 == mFile) {
        mFlag |= (EHF_CLOSING | EHF_CLOSE);
        return EE_NO_OPEN;
    }

    return mLoop->openHandle(this);
}


s32 HandleFile::write(RequestFD* req, usz offset) {
    if (0 == (EHF_WRITEABLE & mFlag)) {
        return EE_NO_WRITEABLE;
    }
    DASSERT(req && req->mCall);
    req->mError = 0;
    req->mType = ERT_WRITE;
    req->mHandle = this;

    //io_submit
    //io_getevents
    //TODO>>

    mLoop->bindFly(this);

    return EE_OK;
}


s32 HandleFile::read(RequestFD* req, usz offset) {
    DASSERT(req && req->mCall);
    if (req->mUsed + sizeof(usz) > req->mAllocated) {
        return EE_ERROR;
    }
    if (0 == (EHF_READABLE & mFlag)) {
        return EE_NO_READABLE;
    }
    req->mError = 0;
    req->mType = ERT_READ;
    req->mHandle = this;
    *(ssz*)(req->mData + req->mUsed) = offset;

    //io_submit
    //TODO>>

    mLoop->bindFly(this);
    if (!Engine::getInstance().getThreadPool().postTask(&HandleFile::readByPool, this, req)) {
        mLoop->unbindFly(this);
        return EE_ERROR;
    }
    return EE_OK;
}


#if defined(DOS_LINUX) || defined(DOS_ANDROID)

//called by thread pool
void HandleFile::readByPool(RequestFD* it) {
    ssz offset = *(usz*)(it->mData + it->mUsed);
    ssz ret = pread64(mFile, it->mData + it->mUsed, it->mAllocated - it->mUsed, offset);
    if (ret > 0) {
        it->mUsed += ret;
    } else if (ret < 0) {
        it->mError = System::getAppError();
    }

    CommandTask task;
    task.pack(&HandleFile::doneByPool, this, it);
    if (EE_OK != mLoop->postTask(task)) {
    }
}

//called by loop thread
void HandleFile::doneByPool(RequestFD* it) {
    it->mCall(it);
    mLoop->unbindFly(this);
}

#endif

}//namespace app
