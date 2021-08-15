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
#include <aio.h>
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

//TODO>>

namespace app {

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

    mFile = ::open(mFilename.c_str(), O_NONBLOCK | O_CREAT | O_RDWR | O_DIRECT, 0644);

    if (0 == mFile) {
        return EE_NO_OPEN;
    }

    /*LARGE_INTEGER fsize;
    if (TRUE == GetFileSizeEx(mFile, &fsize)) {
        mFileSize = fsize.QuadPart;
    }*/
    //s32 efd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    //TODO>>

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
    if (0 == (EHF_READABLE & mFlag)) {
        return EE_NO_READABLE;
    }
    req->mError = 0;
    req->mType = ERT_READ;
    req->mHandle = this;


    //io_submit
    //TODO>>


    mLoop->bindFly(this);
    return EE_OK;
}


}//namespace app
