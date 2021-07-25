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


#include "FileReader.h"
#include <stdio.h>
#if defined(DOS_LINUX)
#include <sys/stat.h>
#endif

namespace app {


FileReader::FileReader() :
    mFile(nullptr),
    mLastSaveTime(0),
    mFileSize(0) {
}

FileReader::~FileReader() {
    close();
}

void FileReader::close() {
    if (mFile) {
        fclose(mFile);
        mFile = nullptr;
        mLastSaveTime = 0;
    }
}


u64 FileReader::read(void* buffer, u64 iSize) {
    if (!isOpen() || nullptr == buffer) {
        return 0;
    }
    return fread(buffer, 1, iSize, mFile);
}


bool FileReader::seek(s64 finalPos, bool relativeMovement) {
    if (!isOpen()) {
        return false;
    }
#if defined(DOS_WINDOWS)
    return _fseeki64(mFile, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    return fseeko64(mFile, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
#endif
}

s64 FileReader::getPos() const {
#if defined(DOS_WINDOWS)
    return _ftelli64(mFile);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    return ftello64(mFile);
#endif
}

bool FileReader::openFile(const String& fileName) {
    if (0 == fileName.getLen()) {
        return false;
    }
    close();
    mFilename = fileName;

    //mFile = _wfopen(mFilename.c_str(), L"rb");
    //mFile = fopen(mFilename.c_str(), "rb");
#if defined(DOS_WINDOWS)
    tchar tmp[_MAX_PATH];
#if defined(DWCHAR_SYS)
    usz len = AppUTF8ToWchar(mFilename.c_str(), tmp, sizeof(tmp));
    s32 ecode = _wfopen_s(&mFile, tmp, DSTR("rb"));
#else
    usz len = AppUTF8ToGBK(mFilename.c_str(), tmp, sizeof(tmp));
    s32 ecode = fopen_s(&mFile, tmp, "rb");
#endif
    if (0 != ecode) {
        mFile = nullptr;
    }
    /*struct _stat64 statbuf;
    if (0 == _wstati64(tmp, &statbuf)) {
        mLastSaveTime = statbuf.st_mtime;
    }*/
#else
    mFile = fopen(mFilename.c_str(), "rb");
#endif

    if (mFile) {
#if defined(DOS_WINDOWS)
        _fseeki64(mFile, 0, SEEK_END);
        mFileSize = _ftelli64(mFile);
        _fseeki64(mFile, 0, SEEK_SET);

        s32 fd2 = _fileno(mFile);
        struct stat statbuf;
        if (0 == fstat(fd2, &statbuf)) {
            mLastSaveTime = statbuf.st_mtime;
        }
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
        fseeko64(mFile, 0, SEEK_END);
        mFileSize = ftello64(mFile);
        fseeko64(mFile, 0, SEEK_SET);

        s32 fd2 = fileno(mFile);
        struct stat statbuf;
        if (0 == fstat(fd2, &statbuf)) {
            mLastSaveTime = statbuf.st_mtim.tv_sec;
        }
#endif
    }
    return nullptr != mFile;
}


} // end namespace app

