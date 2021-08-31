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


#include <string>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "MapFile.h"
#include "System.h"
#include "Logger.h"

namespace app {


MapFile::MapFile() :
    mFile(-1),
    mMemory(nullptr),
    mMemSize(0),
    mFileSize(0),
    mFlag(0) {

    mFileName[0] = 0;
    mMemName[0] = 0;
}


MapFile::~MapFile() {
    closeAll();
}


usz MapFile::getFileSize()const {
    usz fsz = 0;
    if (-1 != mFile) {
        struct stat statbuff;
        if (0 == fstat(mFile, &statbuff)) {
            fsz = statbuff.st_size;
        }
    }
    return fsz;
}


void MapFile::closeAll() {
    if (0 == mFlag) {
        return;
    }
    if (mMemory) {
        munmap(mMemory, mMemSize);
        mMemory = nullptr;
    }

    if (-1 != mFile) {
        close(mFile);
        mFile = -1;
        if (EMF_NEED_DEL & mFlag) {
            remove(mFileName);
        }
    }

    mFileName[0] = 0;
    mMemName[0] = 0;
    mMemSize = 0;
    mFileSize = 0;
    mFlag = 0;
}


bool MapFile::createFile(usz iSize) {
    tchar* realname;
#if defined(DWCHAR_SYS)
    tchar fname[260];
    AppUTF8ToWchar(mFileName, fname, sizeof(fname));
    realname = fname;
#else
    realname = mFileName;
#endif
    if (0 != System::createPath(realname)) {
        Logger::log(ELL_ERROR, "MapFile::createFile>>createPath fail = %s", mFileName);
        return false;
    }
    if (EMF_WRITE & mFlag) {
        mFile = open(realname, O_CREAT | O_RDWR, 00777);
    } else {
        mFile = open(realname, O_CREAT | O_RDONLY, 00777);
    }
    if (-1 == mFile) {
        Logger::log(ELL_ERROR, "MapFile::createFile>>fail = %s", mFileName);
        return false;
    }
    mFileSize = getFileSize();
    if (mFileSize > iSize) {
        iSize = mFileSize;
    }
    usz pagesz = System::getPageSize();
    iSize = ((iSize + pagesz - 1) / pagesz) * pagesz;
    if (mFileSize < iSize) {
        s8 tmp[1024];
        memset(tmp, 0, sizeof(tmp));
        s64 wsz;
        while (mFileSize < iSize) {
            wsz = iSize - mFileSize >= sizeof(tmp) ? sizeof(tmp) : iSize - mFileSize;
            wsz = write(mFile, tmp, wsz);
            if (wsz > 0) {
                mFileSize += wsz;
            } else {
                Logger::log(ELL_ERROR, "MapFile::createFile>>Write fail = %s", mFileName);
                return false;
            }
        }
    }
    return true;
}


bool MapFile::flush() {
    if (mMemory) {
        msync(mMemory, mMemSize, MS_ASYNC);
    }
    return false;
}


void* MapFile::createMem(usz iSize, const s8* iMapName, bool iReadOnly, bool share) {
    if (!iMapName || 0 == iMapName[0]) {
        return nullptr;
    }
    closeAll();
    mFlag = EMF_READ | EMF_CREATOR;
    if (!iReadOnly) {
        mFlag |= EMF_WRITE;
    }
    if (share) {
        mFlag |= EMF_SHARE;
    }
    snprintf(mMemName, sizeof(mMemName), iMapName);
    if (!createMap(mMemName, iSize)) {
        closeAll();
    }
    return mMemory;
}


void* MapFile::openMem(const s8* iMapName, bool iReadOnly) {
    if (!iMapName || 0 == iMapName[0]) {
        return nullptr;
    }
    closeAll();
    mFlag = EMF_READ;
    if (!iReadOnly) {
        mFlag |= EMF_WRITE;
    }
    snprintf(mMemName, sizeof(mMemName), iMapName);
    if (!openMap(mMemName)) {
        closeAll();
    }
    return mMemory;
}


void* MapFile::createMapfile(usz iSize, const s8* iFileName, bool iReadOnly, bool needDEL, bool share) {
    if (!iFileName || 0 == iFileName[0]) {
        return nullptr;
    }
    closeAll();
    mFlag = EMF_READ | EMF_CREATOR;
    if (!iReadOnly) {
        mFlag |= EMF_WRITE;
    }
    if (share) {
        mFlag |= EMF_SHARE;
    }
    if (needDEL) {
        mFlag |= EMF_NEED_DEL;
    }
    snprintf(mFileName, sizeof(mFileName), iFileName);
    String mmm = mFileName;
    mmm.deletePathFromFilename();
    snprintf(mMemName, sizeof(mMemName), mmm.c_str());
    if (createFile(iSize)) {
        createMap(mMemName, mFileSize);
    }
    if (!mMemory) {
        closeAll();
    }
    return mMemory;
}


bool MapFile::createMap(const s8* iMapName, usz iSize) {
    tchar* realname;
#if defined(DWCHAR_SYS)
    tchar fname[sizeof(mMemName)];
    AppUTF8ToWchar(mMemName, fname, sizeof(fname));
    realname = fname;
#else
    realname = mMemName;
#endif

    s32 flag = (EMF_WRITE & mFlag) ? PROT_READ | PROT_WRITE : PROT_READ;
    s32 flag2 = (EMF_SHARE & mFlag) ? MAP_SHARED : MAP_PRIVATE;
    flag2 |= (-1 == mFile) ? MAP_ANON : 0;
    void* ret = mmap(nullptr, iSize, flag, flag2, mFile, 0);
    if (MAP_FAILED == ret) {
        Logger::logError("mmap(%llu) failed,ecode=%d", iSize, System::getAppError());
        return false;
    }
    mMemory = ret;
    mMemSize = iSize;
    return true;
}

bool MapFile::openMap(const s8* iMapName) {
    tchar* realname;
#if defined(DWCHAR_SYS)
    tchar fname[sizeof(mMemName)];
    AppUTF8ToWchar(mMemName, fname, sizeof(fname));
    realname = fname;
#else
    realname = mMemName;
#endif
    s32 flag = (EMF_WRITE & mFlag) ? PROT_WRITE : PROT_READ;
    s32 flag2 = (EMF_SHARE & mFlag) ? MAP_SHARED : MAP_PRIVATE;
    flag2 |= (-1 == mFile) ? MAP_ANON : 0;
    void* ret = mmap(nullptr, 0, flag, flag2, mFile, 0);
    if (MAP_FAILED == ret) {
        Logger::logError("MapFile::openMap>>(%llu) failed,ecode=%d", 0, System::getAppError());
        return false;
    }
    mMemory = ret;
    mMemSize = 0;
    return true;
}

}//app
