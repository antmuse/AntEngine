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


#include "HandleFile.h"
#include "Engine.h"
#include "System.h"
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace app {

static usz AppGetFileSize(s32 fd) {
    usz filesize = 0;
    struct stat statbuff;
    if (fstat(fd, &statbuff) < 0) {
        return filesize;
    } else {
        filesize = (S_ISDIR(statbuff.st_mode)) ? 0 : statbuff.st_size;
    }
    return filesize;
}


HandleFile::HandleFile() : mFile(-1), mFileSize(0) {
    mType = EHT_FILE;
    mLoop = &Engine::getInstance().getLoop();
}


HandleFile::~HandleFile() {
    close();
}


s32 HandleFile::close() {
    if (-1 != mFile) {
        ::close(mFile);
        mFile = -1;
        mFilename.setLen(0);
        mFileSize = 0;
    }
    return EE_OK;
}

bool HandleFile::setFileSize(usz fsz) {
    if (0 == ftruncate(mFile, fsz)) {
        mFileSize = fsz;
    } else {
        Logger::log(ELL_ERROR, "HandleFile::setFileSize>> ecode=%d, filesz=%lu, file=%s", System::getAppError(), fsz,
            mFilename.c_str());
    }
    return true;
}

s32 HandleFile::open(const String& fname, s32 flag) {
    close();
    mFilename = fname;

    s32 fmode = (flag & 2) > 0 ? (O_RDWR) : O_RDONLY;

    if (flag & 4) {
        fmode |= O_CREAT;
        mFile = ::open(mFilename.c_str(), fmode, 0644);
    } else {
        mFile = ::open(mFilename.c_str(), fmode);
    }

    if (-1 == mFile) {
        mFlag |= (EHF_CLOSING | EHF_CLOSE);
        Logger::log(ELL_ERROR, "HandleFile::open>> mode=0x%X, ecode=%d, file=%s", fmode, System::getAppError(),
            mFilename.c_str());
        return EE_NO_OPEN;
    }
    mFileSize = AppGetFileSize(mFile);
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
    req->mOffset = offset;

#if defined(DUSE_IO_URING)
    return mLoop->postRequest(req);
#else
    // using thread pool for file operation
    mLoop->bindFly(this);
    if (!Engine::getInstance().getThreadPool().postTask(&HandleFile::stepByPool, this, req)) {
        mLoop->unbindFly(this);
        return EE_ERROR;
    }
    return EE_OK;
#endif
}


s32 HandleFile::read(RequestFD* req, usz offset) {
    DASSERT(req && req->mCall);
    if (req->mUsed >= req->mAllocated) {
        return EE_ERROR;
    }
    if (0 == (EHF_READABLE & mFlag)) {
        return EE_NO_READABLE;
    }
    req->mError = 0;
    req->mType = ERT_READ;
    req->mHandle = this;
    req->mOffset = offset;

#if defined(DUSE_IO_URING)
    return mLoop->postRequest(req);
#else
    // using thread pool for file operation
    mLoop->bindFly(this);
    if (!Engine::getInstance().getThreadPool().postTask(&HandleFile::stepByPool, this, req)) {
        mLoop->unbindFly(this);
        return EE_ERROR;
    }
    return EE_OK;
#endif
}


#if !defined(DUSE_IO_URING)

// called by thread pool
void HandleFile::stepByPool(RequestFD* it) {
    ssz offset = (ssz)(it->mOffset);
    ssz ret = -1;
    if (ERT_READ == it->mType) {
        ret = pread64(mFile, it->mData + it->mUsed, it->mAllocated - it->mUsed, offset);
    } else {
        ret = pwrite64(mFile, it->mData + it->mUsed, it->mAllocated - it->mUsed, offset);
        DASSERT(it->mAllocated - it->mUsed == ret);
    }
    if (ret > 0) {
        it->mUsed += ret;
    } else if (ret < 0) {
        it->mError = System::getAppError();
        Logger::log(
            ELL_ERROR, "HandleFile::stepByPool>> ecode=%d, offset=%lu, file=%s", it->mError, offset, mFilename.c_str());
    }

    if (EE_OK != mLoop->postTask(&HandleFile::stepByLoop, this, it)) {
        it->mError = System::getAppError();
        Logger::log(ELL_ERROR, "HandleFile::stepByPool>> post ecode=%d, offset=%lu, file=%s", it->mError, offset,
            mFilename.c_str());
        
        mFlag &= ~(EHF_READABLE | EHF_WRITEABLE);
        // it->mCall(it);   now, call here is not thread safe for user
    }
}

// called by loop thread
void HandleFile::stepByLoop(RequestFD* it) {
    if (0 != it->mError) {
        mFlag &= ~(EHF_READABLE | EHF_WRITEABLE);
    }
    it->mCall(it);
    mLoop->unbindFly(this);
}

#endif

} // namespace app
