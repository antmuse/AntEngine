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
#include "System.h"
#include "Engine.h"


namespace app {

HandleFile::HandleFile()
    :mFile(INVALID_HANDLE_VALUE)
    , mFileSize(0) {
    mType = EHT_FILE;
    mLoop = &Engine::getInstance().getLoop();
}


HandleFile::~HandleFile() {
    close();
}


s32 HandleFile::close() {
    if (INVALID_HANDLE_VALUE != mFile) {
        CloseHandle(mFile);
        mFile = INVALID_HANDLE_VALUE;
        mFilename.setLen(0);
        mFileSize = 0;
    }
    return EE_OK;
}


bool HandleFile::setFileSize(usz fsz) {
    if (INVALID_HANDLE_VALUE != mFile) {
        LARGE_INTEGER pos;
        pos.QuadPart = fsz;
        if (TRUE == SetFilePointerEx(mFile, pos, nullptr, FILE_BEGIN)) {
            if (TRUE == SetEndOfFile(mFile)) {
                mFileSize = pos.QuadPart;
                return true;
            }
        }
    }
    return false;
}

s32 HandleFile::open(const String& fname, s32 flag) {
    close();
    mFilename = fname;

    tchar tmp[_MAX_PATH];
#if defined(DWCHAR_SYS)
    usz len = AppUTF8ToWchar(fname.c_str(), tmp, sizeof(tmp));
#else
    usz len = AppUTF8ToGBK(fname.c_str(), tmp, sizeof(tmp));
#endif


    DWORD fmode = (flag & 1) > 0 ? FILE_SHARE_READ : 0;
    fmode |= (flag & 2) > 0 ? FILE_SHARE_WRITE : 0;

    /*
     * CREATE_NEW 创建文件；如文件存在则会出错
     * CREATE_ALWAYS 创建文件，会改写前一个文件
     * OPEN_EXISTING 文件必须已经存在。由设备提出要求
     * OPEN_ALWAYS 如文件不存在则创建它
     * TRUNCATE_EXISTING 将现有文件缩短为零长度
     */
    DWORD cmod = (flag & 4) > 0 ? OPEN_ALWAYS : OPEN_EXISTING;

    /*
     * 使用FILE_FLAG_NO_BUFFERING时，有严格要求，
     * 读写起始偏移量及写入大小，需为磁盘扇区大小整数倍，否则失败
     * https://docs.microsoft.com/en-us/windows/win32/fileio/file-buffering
     */
    DWORD attr = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED
        | FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_WRITE_THROUGH;

    mFile = CreateFile(tmp, GENERIC_READ | ((flag & (2 | 4)) > 0 ? GENERIC_WRITE : 0),
        fmode, nullptr, cmod, attr, nullptr);

    if (INVALID_HANDLE_VALUE == mFile) {
        mFlag |= (EHF_CLOSING | EHF_CLOSE);
        return EE_NO_OPEN;
    }

    LARGE_INTEGER fsize;
    if (TRUE == GetFileSizeEx(mFile, &fsize)) {
        mFileSize = fsize.QuadPart;
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

    req->clearOverlap();
    req->mOverlapped.Pointer = (void*)offset;

    if (FALSE == WriteFile(mFile, req->mData, req->mUsed, nullptr, &req->mOverlapped)) {
        const s32 ecode = System::getError();
        if (ERROR_IO_PENDING != ecode) {
            return mLoop->closeHandle(this);
        }
    }
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

    req->clearOverlap();
    req->mOverlapped.Pointer = (void*)offset;

    if (FALSE == ReadFile(mFile, req->mData + req->mUsed,
        req->mAllocated - req->mUsed, nullptr, &req->mOverlapped)) {
        const s32 ecode = System::getError();
        if (ERROR_IO_PENDING != ecode) {
            return mLoop->closeHandle(this);
        }
    }
    mLoop->bindFly(this);
    return EE_OK;
}


}//namespace app
