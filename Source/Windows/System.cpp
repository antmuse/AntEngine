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


#include "System.h"
#if defined(DOS_WINDOWS)
#include <winsock2.h>
#include <io.h>
#include <tchar.h>
#include "Logger.h"
#include "Windows/WinAPI.h"
#include "Net/Socket.h"

namespace app {

class FileFinder {
public:
    FileFinder() : mHandle(-1) {
        mFormat[0] = 0;
    }

    ~FileFinder() {
        close();
    }

    _tfinddata64_t* open(const tchar* fmt) {
        close();
        _tcscpy_s(mFormat, fmt);
        mHandle = _tfindfirst64(mFormat, &mDat);
        return mHandle != -1 ? &mDat : nullptr;
    }

    _tfinddata64_t* next() {
        return (mHandle != -1 && 0 == _tfindnext64(mHandle, &mDat)) ? &mDat : nullptr;
    }

    void close() {
        if (mHandle != -1) {
            _findclose(mHandle);
            mHandle = -1;
        }
    }

    const tchar* getFormat() const {
        return mFormat;
    }

private:
    intptr_t mHandle;
    _tfinddata64_t mDat;
    tchar mFormat[_MAX_PATH];
};


//@param disk NULL表示程序当前磁盘
static u32 AppGetDiskSectorSize(const tchar* disk = nullptr /*DSTR("C:\\")*/) {
    DWORD sectorsPerCluster;     // 磁盘一个簇内的扇区数
    DWORD bytesPerSector;        // 磁盘一个扇区内的字节数
    DWORD numberOfFreeClusters;  // 磁盘总簇数
    DWORD totalNumberOfClusters; // 磁盘的剩余簇数
    if ((TRUE
            == GetDiskFreeSpace(
                nullptr, &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters))) {
        return bytesPerSector;
    }
    return 0;
}

static u32 AppGetCoreCount() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

static u32 AppGetPageSize() {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwPageSize;
}

s32 System::gSignal = 0;

System::System() {
}

System::~System() {
}

s32 System::getPID() {
    return ::GetCurrentProcessId();
}

u32 System::getCoreCount() {
    static u32 ret = AppGetCoreCount();
    return ret;
}

u32 System::getPageSize() {
    static u32 ret = AppGetPageSize();
    return ret;
}

u32 System::getDiskSectorSize() {
    static u32 ret = AppGetDiskSectorSize();
    return ret;
}

s32 System::convNtstatus2NetError(s32 status) {
    switch (status) {
    case STATUS_SUCCESS:
        return ERROR_SUCCESS;

    case STATUS_PENDING:
        return ERROR_IO_PENDING;

    case STATUS_INVALID_HANDLE:
    case STATUS_OBJECT_TYPE_MISMATCH:
        return WSAENOTSOCK;

    case STATUS_INSUFFICIENT_RESOURCES:
    case STATUS_PAGEFILE_QUOTA:
    case STATUS_COMMITMENT_LIMIT:
    case STATUS_WORKING_SET_QUOTA:
    case STATUS_NO_MEMORY:
    case STATUS_QUOTA_EXCEEDED:
    case STATUS_TOO_MANY_PAGING_FILES:
    case STATUS_REMOTE_RESOURCES:
        return WSAENOBUFS;

    case STATUS_TOO_MANY_ADDRESSES:
    case STATUS_SHARING_VIOLATION:
    case STATUS_ADDRESS_ALREADY_EXISTS:
        return WSAEADDRINUSE;

    case STATUS_LINK_TIMEOUT:
    case STATUS_IO_TIMEOUT:
    case STATUS_TIMEOUT:
        return WSAETIMEDOUT;

    case STATUS_GRACEFUL_DISCONNECT:
        return WSAEDISCON;

    case STATUS_REMOTE_DISCONNECT:
    case STATUS_CONNECTION_RESET:
    case STATUS_LINK_FAILED:
    case STATUS_CONNECTION_DISCONNECTED:
    case STATUS_PORT_UNREACHABLE:
    case STATUS_HOPLIMIT_EXCEEDED:
        return WSAECONNRESET;

    case STATUS_LOCAL_DISCONNECT:
    case STATUS_TRANSACTION_ABORTED:
    case STATUS_CONNECTION_ABORTED:
        return WSAECONNABORTED;

    case STATUS_BAD_NETWORK_PATH:
    case STATUS_NETWORK_UNREACHABLE:
    case STATUS_PROTOCOL_UNREACHABLE:
        return WSAENETUNREACH;

    case STATUS_HOST_UNREACHABLE:
        return WSAEHOSTUNREACH;

    case STATUS_CANCELLED:
    case STATUS_REQUEST_ABORTED:
        return WSAEINTR;

    case STATUS_BUFFER_OVERFLOW:
    case STATUS_INVALID_BUFFER_SIZE:
        return WSAEMSGSIZE;

    case STATUS_BUFFER_TOO_SMALL:
    case STATUS_ACCESS_VIOLATION:
        return WSAEFAULT;

    case STATUS_DEVICE_NOT_READY:
    case STATUS_REQUEST_NOT_ACCEPTED:
        return WSAEWOULDBLOCK;

    case STATUS_INVALID_NETWORK_RESPONSE:
    case STATUS_NETWORK_BUSY:
    case STATUS_NO_SUCH_DEVICE:
    case STATUS_NO_SUCH_FILE:
    case STATUS_OBJECT_PATH_NOT_FOUND:
    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_UNEXPECTED_NETWORK_ERROR:
        return WSAENETDOWN;

    case STATUS_INVALID_CONNECTION:
        return WSAENOTCONN;

    case STATUS_REMOTE_NOT_LISTENING:
    case STATUS_CONNECTION_REFUSED:
        return WSAECONNREFUSED;

    case STATUS_PIPE_DISCONNECTED:
        return WSAESHUTDOWN;

    case STATUS_CONFLICTING_ADDRESSES:
    case STATUS_INVALID_ADDRESS:
    case STATUS_INVALID_ADDRESS_COMPONENT:
        return WSAEADDRNOTAVAIL;

    case STATUS_NOT_SUPPORTED:
    case STATUS_NOT_IMPLEMENTED:
        return WSAEOPNOTSUPP;

    case STATUS_ACCESS_DENIED:
        return WSAEACCES;

    default:
        if ((status & (FACILITY_NTWIN32 << 16)) == (FACILITY_NTWIN32 << 16)
            && (status & (ERROR_SEVERITY_ERROR | ERROR_SEVERITY_WARNING))) {
            // It's a windows error that has been previously mapped to an ntstatus code.
            return (DWORD)(status & 0xffff);
        } else {
            // The default fallback for unmappable ntstatus codes.
            return WSAEINVAL;
        }
    }
}

s32 System::loadNetLib() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData);
}


s32 System::unloadNetLib() {
    return WSACleanup();
}

s32 System::getNetError() {
    return WSAGetLastError();
}

s32 System::getError() {
    return GetLastError();
}

void System::setError(s32 it) {
    SetLastError(it);
}

s32 System::getAppError(s32 err) {
    switch (err) {
    case ERROR_SUCCESS:
        DASSERT(0);
        return EE_ERROR;
    case WSAEINTR:
        return EE_INTR;
    case WAIT_TIMEOUT:
    case WSAETIMEDOUT:
        return EE_TIMEOUT;
    case WSAEWOULDBLOCK:
        return EE_RETRY;
    case WSA_IO_PENDING:
        return EE_POSTED;
    default:
        return err;
    }
}

s32 System::getAppError() {
    return getAppError(getNetError());
}

s32 System::daemon() {
    // not support windows
    return EE_OK;
}

s32 System::createPath(const String& it) {
    if (it.getLen() >= _MAX_PATH) {
        return EE_ERROR;
    }
    tchar fname[_MAX_PATH];
#if defined(DWCHAR_SYS)
    AppUTF8ToWchar(it.c_str(), fname, sizeof(fname));
#else
    AppUTF8ToGBK(it.c_str(), fname, sizeof(fname));
#endif

    DWORD dwAttributes;
    for (tchar* curr = fname; *curr; ++curr) {
        if (isPathDiv(*curr)) {
            if (curr <= fname) {
                return EE_ERROR;
            }
            if (DSTR(':') == curr[-1]) {
                continue;
            }
            tchar bk = *curr;
            *curr = DSTR('\0');
            dwAttributes = GetFileAttributes(fname);
            if (0xffffffff == dwAttributes) {
                if (FALSE == CreateDirectory(fname, NULL)) {
                    if (GetLastError() != ERROR_ALREADY_EXISTS) {
                        return System::getAppError();
                    }
                }
            } else {
                if ((dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
                    return System::getAppError();
                }
            }
            *curr = bk;
        }
    }
    return EE_OK;
}


s32 System::removeFile(const String& it) {
    tchar fname[260];
#if defined(DWCHAR_SYS)
    AppUTF8ToWchar(it.c_str(), fname, sizeof(fname));
#else
    AppUTF8ToGBK(it.c_str(), fname, sizeof(fname));
#endif
    return TRUE == DeleteFile(fname) ? EE_OK : System::getAppError();
}

s32 System::isExist(const String& it) {
    tchar fname[260];
#if defined(DWCHAR_SYS)
    AppUTF8ToWchar(it.c_str(), fname, sizeof(fname));
#else
    AppUTF8ToGBK(it.c_str(), fname, sizeof(fname));
#endif
    DWORD attr = GetFileAttributes(fname);
    if (INVALID_FILE_ATTRIBUTES == attr) {
        return -1;
    }
    return (FILE_ATTRIBUTE_DIRECTORY & attr) > 0 ? 1 : 0;
}


void System::getPathNodes(const String& pth, usz pos, TVector<FileInfo>& out) {
    tchar fname[260];
    usz len;
#if defined(DWCHAR_SYS)
    len = AppUTF8ToWchar(pth.c_str(), fname, sizeof(fname));
#else
    len = AppUTF8ToGBK(pth.c_str(), fname, sizeof(fname));
#endif
    if (0 == len || len + 3 > 260) {
        return;
    }
    if (!isPathDiv(fname[len - 1])) {
        fname[len++] = DSTR('\\');
    }
    fname[len++] = DSTR('*');
    fname[len] = 0;

    FileFinder fnd;
    FileInfo itm;
    const s8* prefix = pth.c_str() + pos;
    if (pos < pth.getLen()) {
        pos = pth.getLen() - pos;
    } else {
        prefix = pth.c_str();
        pos = 0;
    }
    usz addlen = sizeof(itm.mFileName) - pos;
    for (_tfinddata64_t* nd = fnd.open(fname); nd; nd = fnd.next()) {
        if ('.' == nd->name[0]) {
            if (0 == nd->name[1] || ('.' == nd->name[1] || 0 == nd->name[2])) {
                continue;
            }
        }
        memcpy(itm.mFileName, prefix, pos);
#if defined(DWCHAR_SYS)
        len = pos + AppWcharToUTF8(nd->name, itm.mFileName + pos, addlen);
#else
        len = pos + AppGBKToUTF8(nd->name, itm.mFileName + pos, addlen);
#endif
        if ((FILE_ATTRIBUTE_DIRECTORY & nd->attrib) > 0) {
            itm.mFlag = 1;
            itm.mFileName[len++] = '/';
            itm.mFileName[len] = '\0';
        } else {
            itm.mFlag = 0;
        }
        itm.mSize = nd->size;
        itm.mLastSaveTime = nd->time_write;
        out.pushBack(itm);
    }
}


void System::waitProcess(void* handle) {
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
}

s32 System::createProcess(usz socket, void*& handle) {
    handle = nullptr;
    tchar fpath[MAX_PATH + 32];
    u32 flen = GetModuleFileName(NULL, fpath, MAX_PATH);
    if (0 == flen) {
        return 0;
    }
    fpath[flen] = '\0';
    tchar workpath[MAX_PATH];
    tchar title[MAX_PATH];
    memcpy(workpath, fpath, sizeof(workpath));
    for (ssz i = flen; i >= 0; --i) {
        if (workpath[i] == DSTR('\\')) {
            memcpy(title, workpath + i + 1, sizeof(workpath) - sizeof(workpath[0]) * (i + 1));
            workpath[i + 1] = 0;
            break;
        }
    }
    memcpy(fpath + flen, DSTR(" -fork"), sizeof(DSTR(" -fork")));

    STARTUPINFO startinfo;
    ::GetStartupInfo(&startinfo); // take defaults from current process
    startinfo.cb = sizeof(startinfo);
    startinfo.lpReserved = NULL;
    startinfo.lpDesktop = NULL;
    startinfo.lpTitle = title;
    startinfo.wShowWindow = SW_HIDE;
    startinfo.dwFlags = STARTF_FORCEOFFFEEDBACK;
    startinfo.cbReserved2 = 0;
    startinfo.lpReserved2 = NULL;
    startinfo.hStdOutput = 0;
    startinfo.hStdInput = 0;
    startinfo.hStdError = 0;

    HANDLE hProc = ::GetCurrentProcess();
    bool mustInheritHandles = false;
    if (DINVALID_SOCKET != socket) {
        ::DuplicateHandle(hProc, (HANDLE)socket, hProc, &startinfo.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
        mustInheritHandles = true;
        closesocket(socket);
    } else if (GetStdHandle(STD_INPUT_HANDLE)) {
        ::DuplicateHandle(
            hProc, GetStdHandle(STD_INPUT_HANDLE), hProc, &startinfo.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
        mustInheritHandles = true;
    }

    /*if (GetStdHandle(STD_OUTPUT_HANDLE)) {
        ::DuplicateHandle(hProc, GetStdHandle(STD_OUTPUT_HANDLE), hProc, &startinfo.hStdOutput, 0, TRUE,
    DUPLICATE_SAME_ACCESS); mustInheritHandles = true;
    }
    if (GetStdHandle(STD_ERROR_HANDLE)) {
        ::DuplicateHandle(hProc, GetStdHandle(STD_ERROR_HANDLE), hProc, &startinfo.hStdError, 0, TRUE,
    DUPLICATE_SAME_ACCESS); mustInheritHandles = true;
    }*/

    if (mustInheritHandles) {
        startinfo.dwFlags |= STARTF_USESTDHANDLES;
    }

    const tchar* pEnv = nullptr;
    PROCESS_INFORMATION pinfo;
    // DWORD creationFlags = ::GetConsoleWindow() ? 0 : CREATE_NO_WINDOW;
    DWORD creationFlags = CREATE_NO_WINDOW;
    BOOL rc = ::CreateProcess(0, fpath,
        0, // process Attributes
        0, // thread Attributes
        mustInheritHandles, creationFlags, (LPVOID)pEnv, workpath, &startinfo, &pinfo);

    if (rc) {
        ::CloseHandle(pinfo.hThread);
        Logger::log(
            ELL_INFO, "System::createProcess>>pid = %d, cmdsock = %llu", pinfo.dwProcessId, (usz)startinfo.hStdInput);

        handle = pinfo.hProcess;
        // WaitForSingleObject(pinfo.hProcess, INFINITE);
        //::CloseHandle(pinfo.hProcess);
    } else { // fail
        pinfo.dwProcessId = 0;
        s32 du = System::getAppError();
        Logger::log(ELL_INFO, "System::createProcess>>fail = %d, cmdsock = %llu", du, (usz)startinfo.hStdInput);
    }
    if (startinfo.hStdInput) {
        ::CloseHandle(startinfo.hStdInput);
    }
    if (startinfo.hStdOutput) {
        ::CloseHandle(startinfo.hStdOutput);
    }
    if (startinfo.hStdError) {
        ::CloseHandle(startinfo.hStdError);
    }
    return pinfo.dwProcessId;
}

String System::getWorkingPath() {
    String wkpath(256);
    tchar tmp[_MAX_PATH];
#if defined(DWCHAR_SYS)
    _wgetcwd(tmp, _MAX_PATH);
    wkpath.reserve(4 * _tcslen(tmp));
    usz len = AppWcharToUTF8(tmp, (s8*)wkpath.c_str(), wkpath.getAllocated());
#else
    _getcwd(tmp, _MAX_PATH);
    wkpath.reserve(2 * DSLEN(tmp));
    usz len = AppGbkToUTF8(tmp, (s8*)wkpath.c_str(), wkpath.getAllocated());
#endif
    wkpath.setLen(len);
    wkpath.replace('\\', '/');
    if ('/' != wkpath.lastChar()) {
        wkpath += '/';
    }
    return wkpath;
}

} // namespace app

#else
#error unsupported OS
#endif // DOS_WINDOWS
