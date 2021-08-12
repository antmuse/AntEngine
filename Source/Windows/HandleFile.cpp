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


namespace app {

HandleFile::HandleFile()
    :mFile(nullptr) {
    mLoop = &Engine::getInstance().getLoop();
}


HandleFile::~HandleFile() {
    close();
}


s32 HandleFile::close() {
    if (INVALID_HANDLE_VALUE != mFile) {
        CloseHandle(mFile);
        mFile = nullptr;
        mFilename.setLen(0);
    }
    return EE_OK;
}


s32 HandleFile::open(const String& fname, s32 flag) {
    mFilename = fname;

    tchar tmp[_MAX_PATH];
#if defined(DWCHAR_SYS)
    usz len = AppUTF8ToWchar(fname.c_str(), tmp, sizeof(tmp));
#else
    usz len = AppUTF8ToGBK(fname.c_str(), tmp, sizeof(tmp));
#endif

    //FILE_APPEND_DATA
    DWORD fmode = (flag & 1) > 0 ? FILE_SHARE_READ : 0;
    fmode |= (flag & 2) > 0 ? FILE_SHARE_WRITE : 0;

    DWORD cmod = 0;
    if ((flag & 4) > 0) {
        cmod = (flag & 8) > 0 ? CREATE_ALWAYS : OPEN_ALWAYS;
    } else {
        cmod = (flag & 8) > 0 ? CREATE_NEW : OPEN_EXISTING;
    }

    mFile = CreateFile(tmp,
        GENERIC_READ | GENERIC_WRITE,
        fmode, //FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        cmod, //OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (INVALID_HANDLE_VALUE == mFile) {
        return EE_NO_OPEN;
    }
    return mLoop->openHandle(this);
}


s32 HandleFile::write(RequestFD* req) {
    DASSERT(req && req->mCall);
    req->mError = 0;
    req->mType = ERT_WRITE;
    req->mHandle = this;

    req->clearOverlap();
    if (TRUE == WriteFileEx(mFile, req->mData + req->mUsed,
        req->mAllocated - req->mUsed, &req->mOverlapped, nullptr)) {

    }
    return EE_OK;
}


s32 HandleFile::read(RequestFD* req) {
    DASSERT(req && req->mCall);
    req->mError = 0;
    req->mType = ERT_READ;
    req->mHandle = this;

    req->clearOverlap();
    if (TRUE == ReadFileEx(mFile, req->mData + req->mUsed,
        req->mAllocated - req->mUsed, &req->mOverlapped, nullptr)) {

    }


    return EE_OK;
}


}//namespace app
