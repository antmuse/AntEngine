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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/sysctl.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "Logger.h"
#include "Engine.h"

namespace app {

s32 System::gSignal = 0;


static u32 AppGetCoreCount() {
    return get_nprocs();
}

static u32 AppGetPageSize() {
    return getpagesize();
}


s32 System::mFatherPID = 0;

System::System() {
    mFatherPID = getPID();
    signal(SIGINT, System::onSignal);
    //signal(SIGILL, System::onSignal);
    signal(SIGTERM, System::onSignal);
    signal(SIGKILL, System::onSignal);
    signal(SIGQUIT, System::onSignal);
    signal(SIGCHLD, System::onSignal);
}

System::~System() {
}


void System::onSignal(s32 val) {
    gSignal = val;
    Logger::log(ELL_INFO, "System::onSignal>>recv signal = %d", val);
    switch (val) {
    case SIGINT: //ctrl+c
    case SIGKILL: //kill
        Engine::getInstance().postCommand(ECT_EXIT);
        break;
    default:
        break;
    }
}

s32 System::getPID() {
    return getpid();
}

s32 System::daemon() {
    s32 pid = fork();
    if (pid < 0) {
        Logger::logError("System::daemon>>fork() failed,ecode=%d", getError());
        return EE_ERROR;
    }
    if (pid > 0) {
        Logger::logInfo("System::daemon>>fork() pid=%d", pid);
        Logger::flush();
        exit(0);
        //return EE_ERROR;
    }

    //child
    mFatherPID = getPID();

    if (setsid() == -1) {
        Logger::logError("System::daemon>>setsid() failed,ecode=%d", getError());
        return EE_ERROR;
    }

    umask(0);

    s32 fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        Logger::logError("System::daemon>>open(\"/dev/null\") failed,ecode=%d", getError());
        return EE_ERROR;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        Logger::logError("System::daemon>>dup2(STDIN) failed,ecode=%d", getError());
        return EE_ERROR;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        Logger::logError("System::daemon>>dup2(STDOUT) failed,ecode=%d", getError());
        return EE_ERROR;
    }

#if 0
    if (dup2(fd, STDERR_FILENO) == -1) {
        Logger::logError("System::daemon>>dup2(STDERR) failed,ecode=%d", getError());
        return EE_ERROR;
    }
#endif

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            Logger::logError("System::daemon>>close(%d) failed,ecode=%d", fd, getError());
            return EE_ERROR;
        }
    }

    return EE_OK;
}

s32 System::loadNetLib() {
    return 0;
}


s32 System::unloadNetLib() {
    return 0;
}

s32 System::getNetError() {
    return errno;
}

s32 System::getError() {
    return errno;
}

void System::setError(s32 it) {
    errno = it;
}


s32 System::getAppError(s32 err) {
    switch (err) {
    case 0:
        DASSERT(0);
        return EE_ERROR;
    case EINTR:
        return EE_INTR;
    case EAGAIN:    //case EWOULDBLOCK:
        return EE_RETRY;
    case EINPROGRESS:
        return EE_POSTED;
    case EMFILE:
        return EE_TOO_MANY_FD;
    case EALREADY:
    default:
        return err;
    }
}

s32 System::getAppError() {
    return getAppError(getError());
}


u32 System::getCoreCount() {
    static u32 ret = AppGetCoreCount();
    return ret;
}


u32 System::getPageSize() {
    static u32 ret = AppGetPageSize();
    return ret;
}

s32 System::createPath(const String& it) {
    if (it.getLen() >= 260) {
        return EE_ERROR;
    }
    tchar fname[260];
    memcpy(fname, it.c_str(), 1 + it.getLen());

    for (tchar* curr = fname; *curr; ++curr) {
        if (isPathDiv(*curr)) {
            if (curr <= fname) {
                continue;
            }
            tchar bk = *curr;
            *curr = DSTR('\0');
            if (0 != access(fname, F_OK)) {
                if (0 != mkdir(fname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
                    return System::getAppError();
                }
            }
            *curr = bk;
        }
    }
    return EE_OK;
}



s32 System::removeFile(const String& it) {
    return remove(it.c_str());
}


s32 System::isExist(const String& it) {
    //return (0 == access(it.c_str(), F_OK)) ? 0 : -1;
    struct stat statbuf;
    if (0 == stat(it.c_str(), &statbuf)) {
        if (0 != (S_IFDIR & statbuf.st_mode)) {
            return 1;
        }
        return (0 != (S_IFREG & statbuf.st_mode)) ? 0 : -1;
    }
    return -1;
}


void System::getPathNodes(const String& pth, usz pos, TVector<FileInfo>& out) {
    tchar fname[260];
    usz len = AppMin<usz>(sizeof(fname), pth.getLen());
    if (0 == len || len + 3 > 260) {
        return;
    }
    memcpy(fname, pth.c_str(), len);
    if (!isPathDiv(fname[len - 1])) {
        fname[len++] = DSTR('\\');
    }
    fname[len] = 0;

    DIR* dir;
    struct dirent* nd;
    if ((dir = opendir(fname)) == nullptr) {
        //("Open dir error...");
        return;
    }

    FileInfo itm;
    const s8* prefix = pth.c_str() + pos;
    if (pos < pth.getLen()) {
        pos = pth.getLen() - pos;
    } else {
        prefix = pth.c_str();
        pos = 0;
    }
    memcpy(itm.mFileName, prefix, pos);
    usz addlen = sizeof(itm.mFileName) - pos;
    while ((nd = readdir(dir)) != nullptr) {
        if ('.' == nd->d_name[0]) {
            if (0 == nd->d_name[1] || ('.' == nd->d_name[1] || 0 == nd->d_name[2])) {
                continue;
            }
        }
        len = strlen(nd->d_name);
        if (len + 2 > addlen) { //too long
            continue;
        }
        memcpy(itm.mFileName + pos, nd->d_name, len + 1);
        len += pos;

        itm.mSize = 0;
        itm.mLastSaveTime = 0;

        if (nd->d_type == 8) {    //file
            itm.mFlag = 0;
            out.pushBack(itm);
        } else if (nd->d_type == 4) {    //dir
            itm.mFlag = 1;
            itm.mFileName[len++] = '/';
            itm.mFileName[len] = '\0';
            out.pushBack(itm);
        } else if (nd->d_type == 10) {    //link file

        }
    }
    closedir(dir);
}


} //namespace app

