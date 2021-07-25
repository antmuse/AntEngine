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


#include "FileWriter.h"
#include <stdio.h>
#include <wchar.h>

namespace app {


FileWriter::FileWriter() :
    mFile(nullptr),
    mFileSize(0) {
}

FileWriter::~FileWriter() {
    close();
}

void FileWriter::close() {
    if (mFile) {
        fclose(mFile);
        mFile = nullptr;
    }
}

bool FileWriter::flush() {
    if (!isOpen()) {
        return false;
    }
    return 0 == fflush(mFile);
}


u64 FileWriter::write(const void* buffer, u64 sizeToWrite) {
    if (!isOpen()) {
        return 0;
    }
    u64 ret = fwrite(buffer, 1, sizeToWrite, mFile);
    mFileSize += ret;
    return ret;
}

u64 FileWriter::writeParams(const s8* format, va_list& args) {
    if (!isOpen()) {
        return 0;
    }
    u64 ret = vfprintf(mFile, format, args);
    mFileSize += ret;
    return ret;
}

u64 FileWriter::writeWParams(const wchar_t* format, va_list& args) {
    if (!isOpen()) {
        return 0;
    }
    u64 ret = vfwprintf(mFile, format, args);
    mFileSize += ret;
    return ret;
}

u64 FileWriter::writeParams(const s8* format, ...) {
    if (!isOpen()) {
        return 0;
    }
    va_list args;
    va_start(args, format);
    u64 ret = vfprintf(mFile, format, args);
    va_end(args);
    mFileSize += ret;
    return ret;
}

u64 FileWriter::writeWParams(const wchar_t* format, ...) {
    if (!isOpen()) {
        return 0;
    }
    va_list args;
    va_start(args, format);
    u64 ret = vfwprintf(mFile, format, args);
    va_end(args);
    mFileSize += ret;
    return ret;
}


bool FileWriter::seek(s64 finalPos, bool relativeMovement) {
    if (!isOpen()) {
        return false;
    }
#if defined(DOS_WINDOWS)
    return _fseeki64(mFile, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    return fseeko64(mFile, finalPos, relativeMovement ? SEEK_CUR : SEEK_SET) == 0;
#endif
}

s64 FileWriter::getPos() const {
#if defined(DOS_WINDOWS)
    return _ftelli64(mFile);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
    return ftello64(mFile);
#endif
}

bool FileWriter::openFile(const String& fileName, bool append) {
    if (0 == fileName.getLen()) {
        return false;
    }
    close();

    mFilename = fileName;
    //mFile = _wfopen(mFilename.c_str(), append ? L"ab" : L"wb");
    //mFile = fopen(mFilename.c_str(), append ? "ab" : "wb");

#if defined(DOS_WINDOWS)
    tchar tmp[_MAX_PATH];
#if defined(DWCHAR_SYS)
    usz len = AppUTF8ToWchar(mFilename.c_str(), tmp, sizeof(tmp));
    s32 ecode = _wfopen_s(&mFile, tmp, append ? DSTR("ab") : DSTR("wb"));
#else
    usz len = AppUTF8ToGBK(mFilename.c_str(), tmp, sizeof(tmp));
    s32 ecode = fopen_s(&mFile, tmp, append ? "ab" : "wb");
#endif
    if (0 != ecode) {
        mFile = nullptr;
    }
#else
    mFile = fopen(mFilename.c_str(), append ? "ab" : "wb");
#endif
    if (mFile) {
#if defined(DOS_WINDOWS)
        _fseeki64(mFile, 0, SEEK_END);
        mFileSize = _ftelli64(mFile);
        _fseeki64(mFile, 0, SEEK_SET);
#elif defined(DOS_LINUX) || defined(DOS_ANDROID)
        fseeko64(mFile, 0, SEEK_END);
        mFileSize = ftello64(mFile);
        fseeko64(mFile, 0, SEEK_SET);
#endif
    }
    return nullptr != mFile;
}


} // end namespace app

