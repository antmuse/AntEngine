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


#include <windows.h>
#include <string>
#include "MapFile.h"
#include "System.h"
#include "Logger.h"

namespace app {

//"Gobal\\%s" 需要跨用户权限(admin)，我们只需要同用户多进程共享
static const s8* G_MEM_FMT = "Local\\%s";

MapFile::MapFile() :
    mFile(INVALID_HANDLE_VALUE),
    mMapHandle(nullptr),
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
    if (mFile != INVALID_HANDLE_VALUE) {
        return GetFileSize(mFile, nullptr);
    }
    return 0;
}


void MapFile::closeAll() {
    if (0 == mFlag) {
        return;
    }
    if (mMemory) {
        UnmapViewOfFile(mMemory);
        mMemory = nullptr;
    }

    if (mMapHandle) {
        CloseHandle(mMapHandle);
        mMapHandle = nullptr;
    }

    if (mFile != INVALID_HANDLE_VALUE) {
        CloseHandle(mFile);
        mFile = INVALID_HANDLE_VALUE;
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
        //path_file_helper::create_dirs(mFileName);
        mFile = CreateFile(realname,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_ALWAYS,
            FILE_FLAG_SEQUENTIAL_SCAN,
            nullptr);
    } else {
        mFile = CreateFile(realname,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_SEQUENTIAL_SCAN,
            nullptr);
    }
    if (INVALID_HANDLE_VALUE == mFile) {
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
        DWORD wsz;
        while (mFileSize < iSize) {
            wsz = (DWORD)(iSize - mFileSize >= sizeof(tmp) ? sizeof(tmp) : iSize - mFileSize);
            if (TRUE == WriteFile(mFile, tmp, (DWORD)wsz, &wsz, nullptr)) {
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
        if (TRUE == FlushViewOfFile(mMemory, 0)) {
            return true;
        }
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

    String mmm = iMapName;
    mmm.deletePathFromFilename();

    snprintf(mMemName, sizeof(mMemName), G_MEM_FMT, mmm.c_str());
    if (createMap(mMemName, iSize)) {
        createView();
        mFlag |= EMF_CREATOR;
    }
    if (!mMemory) {
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
    String mmm = iMapName;
    mmm.deletePathFromFilename();
    snprintf(mMemName, sizeof(mMemName), G_MEM_FMT, mmm.c_str());
    if (openMap(mMemName)) {
        createView();
    }
    if (!mMemory) {
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

    snprintf(mMemName, sizeof(mMemName), G_MEM_FMT, mmm.c_str());
    if (createFile(iSize)) {
        if (createMap(mMemName, mFileSize)) {
            createView();
        }
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
    DWORD DesiredAccess = (EMF_WRITE & mFlag) ? PAGE_READWRITE : PAGE_READONLY;
    if (EMF_SHARE & mFlag) {
        SECURITY_ATTRIBUTES sa;
        SECURITY_DESCRIPTOR sd;
        InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(&sd, TRUE, nullptr, FALSE);
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = &sd;
        sa.bInheritHandle = FALSE;
        mMapHandle = CreateFileMapping(mFile,
            &sa,
            DesiredAccess,
            iSize >> 32,
            iSize & 0xFFFFFFFF,
            realname);
    } else {
        mMapHandle = CreateFileMapping(mFile,
            nullptr,
            DesiredAccess,
            iSize >> 32,
            iSize & 0xFFFFFFFF,
            realname);
    }
    if (mMapHandle == nullptr) {
        Logger::log(ELL_ERROR, "MapFile::createMap>>fail = %s, err=%d, pls run as Administrator", mMemName, System::getAppError());
        return false;
    }
    return true;
}


bool MapFile::createView() {
    DWORD DesiredAccess = (EMF_WRITE & mFlag) ? FILE_MAP_WRITE : FILE_MAP_READ;
    mMemory = MapViewOfFile(mMapHandle, DesiredAccess, 0, 0, 0);
    if (mMemory) {
        MEMORY_BASIC_INFORMATION mi;
        VirtualQuery(mMemory, &mi, sizeof(mi));
        mMemSize = mi.RegionSize;
    }
    return nullptr != mMemory;
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

    u32 DesiredAccess = (EMF_WRITE & mFlag) ? FILE_MAP_WRITE : FILE_MAP_READ;
    mMapHandle = OpenFileMapping(DesiredAccess, FALSE, realname);
    if (mMapHandle == nullptr) {
        Logger::log(ELL_ERROR, "MapFile::openMap>>fail = %s, err=%d, pls run as Administrator", mMemName, System::getAppError());
        return false;
    }
    return mMapHandle != nullptr;
}

}//app